#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
stack_usage.py — SDCC mcs51 静态栈消耗分析（构建时自动运行）

解析 SDCC 为每个链接模块生成的 .asm，重建每个函数的栈行为
（push/pop、bp 帧、sp 调整、调用点），构建调用图，计算：
  main 链最坏深度 + 最大 ISR 叠加 = 最坏栈消耗
并与 --stack-size 比较。

libsdcc 库函数没有 .asm：用 s51 (ucsim) 直接从链接后的 .hex
反汇编，用同一套引擎分析；复杂库函数（printf 链）走 override 表。

退出码：最坏情况 <= --stack-size 时为 0，否则为 1。
"""

import argparse
import json
import os
import re
import subprocess
import sys
from collections import defaultdict

SFR_NAMES = {
    '0x81': 'sp', '0xe0': 'a', '0x82': 'dpl', '0x83': 'dph',
    '0xf0': 'b', '0xd0': 'psw',
}

ANSI_RE = re.compile(r'\x1b\[[0-9;]*[A-Za-z]')


def signed8(v):
    return v - 256 if v >= 128 else v


class FuncInfo:
    def __init__(self, name, peak, call_sites, tail_sites, is_isr, source):
        self.name = name
        self.peak = peak
        self.call_sites = call_sites
        self.tail_sites = tail_sites
        self.is_isr = is_isr
        self.source = source


def analyze_items(name, items, source):
    trampolines = {it[2] for it in items
                   if it[0] == 'I' and it[1] in ('lcall', 'acall')
                   and it[2].endswith('$')}
    depth = 0
    peak = 0
    bp_depth = None
    pending = None
    call_sites = []
    tail_sites = []
    is_isr = False
    skipping = False
    for it in items:
        if it[0] == 'L':
            if it[1] in trampolines:
                skipping = True
            continue
        mnem, op = it[1], it[2]
        if skipping:
            if mnem == 'ret':
                skipping = False
            continue

        if pending is not None:
            if mnem == 'add' and op.startswith('a,#'):
                try:
                    pending = (pending[0], pending[1] + signed8(int(op[3:], 16)))
                except ValueError:
                    pending = None
                if pending is not None:
                    continue
            elif mnem == 'mov' and op == 'sp,a':
                base, delta = pending
                depth = (depth + delta) if base == 'sp' else ((bp_depth or 0) + delta)
                peak = max(peak, depth)
                pending = None
                continue
            else:
                pending = None

        if mnem == 'mov':
            if op == 'a,sp':
                pending = ('sp', 0)
            elif op == 'a,_bp':
                pending = ('bp', 0)
            elif op == '_bp,sp':
                bp_depth = depth
            elif op == 'sp,_bp':
                if bp_depth is not None:
                    depth = bp_depth
            continue
        if mnem == 'push':
            depth += 1
            peak = max(peak, depth)
        elif mnem == 'pop':
            depth -= 1
        elif mnem == 'inc' and op == 'sp':
            depth += 1
            peak = max(peak, depth)
        elif mnem == 'dec' and op == 'sp':
            depth -= 1
        elif mnem in ('lcall', 'acall'):
            target = None if op.endswith('$') else op
            call_sites.append((depth, target))
        elif mnem == 'ljmp' and op and not op.endswith('$'):
            tail_sites.append((depth, op))
        elif mnem == 'reti':
            is_isr = True
    return FuncInfo(name, peak, call_sites, tail_sites, is_isr, source)


FUNC_MARKER = re.compile(r'^;\s*function\s+(\w+)\s*$')
LABEL_RE = re.compile(r'^(\S+):\s*$')


def parse_asm_file(path):
    funcs = {}
    pending_name = None
    cur = None
    with open(path, encoding='utf-8', errors='replace') as fh:
        for raw in fh:
            line = raw.rstrip('\n')
            m = FUNC_MARKER.match(line)
            if m:
                pending_name = m.group(1)
                continue
            m = LABEL_RE.match(line)
            if m:
                label = m.group(1)
                if pending_name is not None and label == '_' + pending_name:
                    cur = []
                    funcs[label] = (cur, path)
                    pending_name = None
                elif cur is not None:
                    cur.append(('L', label))
                continue
            stripped = line.strip()
            if not stripped or stripped.startswith(';'):
                continue
            if stripped.startswith('.'):
                # Only a section change ends the function body; inline data
                # directives (.db jump tables, .globl, ...) appear mid-function.
                if stripped.split()[0].lower() == '.area':
                    cur = None
                continue
            parts = re.split(r'\s+', stripped, maxsplit=1)
            mnem = parts[0].lower()
            op = re.sub(r'\s+', '', parts[1]) if len(parts) > 1 else ''
            if cur is not None:
                cur.append(('I', mnem, op))
    return funcs


def parse_map(path):
    sym2addr = {}
    addr2sym = {}
    pat = re.compile(r'^C:\s*([0-9A-Fa-f]+)\s+(\S+)')
    with open(path, encoding='utf-8', errors='replace') as fh:
        for line in fh:
            m = pat.match(line)
            if not m:
                continue
            addr = int(m.group(1), 16)
            sym = m.group(2)
            if sym not in sym2addr:
                sym2addr[sym] = addr
            if addr not in addr2sym:
                addr2sym[addr] = sym
    return sym2addr, addr2sym


def norm_dis_insn(mnem, op, addr2sym):
    op = ANSI_RE.sub('', op)
    op = re.sub(r'<[^>]*>', '', op)
    op = re.sub(r'\s+', '', op).lower()
    if mnem in ('lcall', 'acall'):
        m = re.fullmatch(r'0x([0-9a-f]+)', op)
        if m:
            sym = addr2sym.get(int(m.group(1), 16))
            return ('I', mnem, sym if sym else '??0x' + m.group(1))
        return ('I', mnem, op)
    for num, name in SFR_NAMES.items():
        op = re.sub(r'\b' + num + r'\b', name, op)
    return ('I', mnem, op)


DC_LINE = re.compile(
    r'^(0x[0-9a-f]+)\s+(?:[0-9a-f]{2}\s)+\s*([A-Za-z]+)\s*(.*)$')


def parse_dc_block(text):
    insns = []
    for line in text.splitlines():
        line = ANSI_RE.sub('', line).strip()
        m = DC_LINE.match(line)
        if not m:
            continue
        insns.append((int(m.group(1), 16), m.group(2).lower(), m.group(3)))
    return insns


VECTOR_ADDRS = {0x03, 0x0B, 0x13, 0x1B, 0x23, 0x2B, 0x33, 0x3B,
                0x43, 0x4B, 0x53, 0x5B}


def detect_vector_isrs(s51, hexfile, addr2sym, warnings):
    try:
        p = run_s51(s51, hexfile, 'dc 0x0000 0x0060\n', 60)
    except (OSError, subprocess.TimeoutExpired) as e:
        warnings.append('s51 向量表反汇编失败: %s' % e)
        return None
    isrs = set()
    for addr, mnem, op in parse_dc_block(p.stdout):
        if addr not in VECTOR_ADDRS or mnem not in ('ljmp', 'lcall', 'ajmp', 'acall'):
            continue
        op = re.sub(r'\s+', '', ANSI_RE.sub('', op)).lower()
        m = re.fullmatch(r'0x([0-9a-f]+)', op)
        if m:
            sym = addr2sym.get(int(m.group(1), 16))
            if sym:
                isrs.add(sym)
    return isrs


def run_s51(s51, hexfile, cmds, timeout):
    d = os.path.dirname(os.path.abspath(hexfile))
    return subprocess.run([s51, '-t', 'C52', os.path.basename(hexfile)],
                          input=cmds + 'quit\n', capture_output=True,
                          text=True, timeout=timeout, cwd=d)


def truncate_at_ret(items):
    out = []
    for it in items:
        out.append(it)
        if it[0] == 'I' and it[1] in ('ret', 'reti'):
            break
    return out


def disassemble_lib_funcs(s51, hexfile, symbols, sym2addr, addr2sym, defined,
                         warnings):
    result = {}
    queue = list(symbols)
    done = set()
    while queue:
        batch = [s for s in queue if s not in done and s in sym2addr]
        queue = []
        if not batch:
            break
        ranges = []
        for s in batch:
            a = sym2addr[s]
            ranges.append((s, a, min(a + 512, 0x10000)))
        cmds = ''.join('dc 0x{:04x} 0x{:04x}\n'.format(a, b) for _, a, b in ranges)
        try:
            p = run_s51(s51, hexfile, cmds, 120)
        except (OSError, subprocess.TimeoutExpired) as e:
            warnings.append('s51 反汇编失败: %s' % e)
            break
        sections = re.split(r'^dc 0x[0-9a-f]+ 0x[0-9a-f]+\s*$', p.stdout, flags=re.M)
        for (s, a, b), block in zip(ranges, sections[1:]):
            items = [norm_dis_insn(m, o, addr2sym) for _, m, o in parse_dc_block(block)]
            items = truncate_at_ret(items)
            info = analyze_items(s, items, 'libsdcc@0x%04X' % a)
            result[s] = info
            done.add(s)
            for _, target in info.call_sites + info.tail_sites:
                if target and target not in result and target not in done \
                        and target not in defined \
                        and not target.startswith('??'):
                    if target in sym2addr:
                        queue.append(target)
                    else:
                        warnings.append('库函数 %s 调用了未解析符号 %s' % (s, target))
        for s in batch:
            done.add(s)
    return result


def main():
    ap = argparse.ArgumentParser(description='SDCC mcs51 static stack usage analyzer')
    ap.add_argument('asm_files', nargs='+')
    ap.add_argument('--map')
    ap.add_argument('--hex')
    ap.add_argument('--s51')
    ap.add_argument('--stack-size', type=int, default=None)
    ap.add_argument('--config')
    ap.add_argument('--entry', default='_main')
    ap.add_argument('--quiet', action='store_true')
    args = ap.parse_args()

    lib_overrides = {'_printf': 68}
    lib_fallback = {
        '__gptrget': 4, '__gptrput': 5, '___gptr_cmp': 2,
        '___memcpy': 5, '_memset': 4, '_strlen': 2,
        '__mulint': 2, '__mullong': 4, '__divulong': 6, '__moduint': 6,
    }
    indirect_targets = {}
    if args.config:
        with open(args.config, encoding='utf-8') as fh:
            cfg = json.load(fh)
        lib_overrides.update(cfg.get('lib_overrides', {}))
        lib_fallback.update(cfg.get('lib_fallback', {}))
        indirect_targets.update(cfg.get('indirect_targets', {}))

    warnings = []
    funcs = {}
    for path in args.asm_files:
        try:
            for name, (items, src) in parse_asm_file(path).items():
                funcs[name] = analyze_items(name, items, src)
        except OSError as e:
            warnings.append('无法读取 %s: %s' % (path, e))

    defined = set(funcs)
    called = set()
    for f in funcs.values():
        for _, t in f.call_sites + f.tail_sites:
            if t:
                called.add(t)
    undefined = sorted(s for s in called - defined
                       if not s.startswith('??'))

    lib_funcs = {}
    used_fallback = []
    vector_isrs = None
    if args.map and args.hex and args.s51:
        sym2addr, addr2sym = parse_map(args.map)
        to_dis = [s for s in undefined
                  if s not in lib_overrides and s != '__sdcc_call_dptr'
                  and s in sym2addr]
        missing = [s for s in undefined
                   if s not in lib_overrides and s != '__sdcc_call_dptr'
                   and s not in sym2addr]
        for s in missing:
            warnings.append('符号 %s 不在 map 中，无法分析' % s)
        lib_funcs = disassemble_lib_funcs(args.s51, args.hex, to_dis,
                                        sym2addr, addr2sym, defined, warnings)
        vector_isrs = detect_vector_isrs(args.s51, args.hex, addr2sym, warnings)
    for s in undefined:
        if s in lib_overrides or s == '__sdcc_call_dptr':
            continue
        if s not in lib_funcs:
            if s in lib_fallback:
                used_fallback.append(s)
            else:
                warnings.append('未知库符号 %s（按 0 计，请补充 override）' % s)

    memo = {}
    choice = {}
    recursions = set()
    unknown_syms = set()

    def cost_of(target, stack):
        if target is None or target == '__sdcc_call_dptr':
            return None
        if target in lib_overrides:
            return lib_overrides[target]
        if target in funcs or target in lib_funcs:
            return resolve(target, stack)
        if target in lib_fallback:
            return lib_fallback[target]
        if target.startswith('??'):
            unknown_syms.add(target)
            return 0
        unknown_syms.add(target)
        return 0

    def resolve(name, stack=()):
        if name in memo:
            return memo[name]
        if name in stack:
            recursions.add(name)
            return 0
        f = funcs.get(name) or lib_funcs.get(name)
        if f is None:
            return 0
        stack2 = stack + (name,)
        total = f.peak
        best = None
        for site_depth, target in f.call_sites:
            if target is None or target == '__sdcc_call_dptr':
                cands = indirect_targets.get(name, [])
                if not cands:
                    warnings.append('%s 存在未声明的间接调用（config indirect_targets）' % name)
                    cost = 0
                else:
                    cost = max((resolve(c, stack2) for c in cands), default=0)
            else:
                cost = cost_of(target, stack2)
                if cost is None:
                    continue
            v = site_depth + 2 + cost
            if v > total:
                total = v
                best = (site_depth, target)
        for site_depth, target in f.tail_sites:
            cost = cost_of(target, stack2)
            if cost is None:
                continue
            v = site_depth + cost
            if v > total:
                total = v
                best = (site_depth, target)
        memo[name] = total
        if best:
            choice[name] = best
        return total

    if vector_isrs:
        isrs = {n: funcs[n] for n in vector_isrs if n in funcs}
        for n in sorted(vector_isrs - set(funcs)):
            warnings.append('向量表 ISR %s 未在解析的 .asm 中找到' % n)
    else:
        isrs = {n: f for n, f in funcs.items() if f.is_isr}

    entry_total = resolve(args.entry)
    main_total = 2 + entry_total

    isr_totals = {}
    for n in isrs:
        isr_totals[n] = 2 + resolve(n)
    max_isr = max(isr_totals.values(), default=0)
    max_isr_name = max(isr_totals, key=isr_totals.get, default=None)

    worst = main_total + max_isr

    out = []
    out.append('=== 静态栈消耗分析 (SDCC mcs51) ===')
    out.append('函数: %d (用户代码) + %d (libsdcc 反汇编)' % (len(funcs), len(lib_funcs)))
    if used_fallback:
        out.append('库函数使用内置估算值: %s' % ', '.join(sorted(used_fallback)))

    rows = []
    for n in list(funcs) + [x for x in lib_funcs if x not in funcs]:
        t = memo.get(n)
        if t is None:
            t = resolve(n)
        f = funcs.get(n) or lib_funcs.get(n)
        rows.append((t, f.peak, n, f.source))
    rows.sort(reverse=True)
    out.append('')
    out.append('%-32s %6s %6s  %s' % ('函数', '局部', '含下级', '来源'))
    for t, pk, n, src in rows[:15]:
        base = src.split('F__simpleDoorLock_STC12_')[-1] if 'F__' in src else src
        out.append('%-32s %6d %6d  %s' % (n, pk, t, base))

    out.append('')
    out.append('main 最坏链（累计深度）:')
    cum = 2
    out.append('  %5d  → %s (启动 lcall)' % (cum, args.entry))
    cur = args.entry
    guard = 0
    while cur in choice and guard < 64:
        guard += 1
        site_depth, target = choice[cur]
        cum += site_depth + 2
        if target is None or target == '__sdcc_call_dptr':
            out.append('  %5d  → [间接调用] (via %s, 调用点深度 %d)' % (cum, cur, site_depth))
            break
        out.append('  %5d  → %s (%s 调用点深度 %d + 2)' % (cum, target, cur, site_depth))
        if target in lib_overrides:
            cum += lib_overrides[target]
            out.append('  %5d     [%s override = %d, 含其全部下级]' % (cum, target, lib_overrides[target]))
            break
        if target not in funcs and target not in lib_funcs:
            break
        cur = target
    else:
        f = funcs.get(cur) or lib_funcs.get(cur)
        if f:
            out.append('  %5d     [%s 局部峰值 %d]' % (cum, cur, f.peak))
    out.append('main 链合计: %d B' % main_total)

    out.append('')
    out.append('ISR 叠加（优先级 0 不嵌套，取最大者）:')
    for n, t in sorted(isr_totals.items(), key=lambda x: -x[1]):
        out.append('  %-24s 2(硬件) + %d = %d B' % (n, t - 2, t))

    out.append('')
    out.append('最坏情况 = main %d + ISR %s %d = %d B' %
               (main_total, max_isr_name or '-', max_isr, worst))
    if args.stack_size is not None:
        margin = args.stack_size - worst
        out.append('预留 STACK_SIZE = %d B → 余量 %d B%s' %
                   (args.stack_size, margin,
                    '' if margin >= 8 else '  ← 余量不足 8 B!' if margin >= 0 else '  ← 溢出!'))

    if recursions:
        warnings.append('检测到递归（按单次计）: %s' % ', '.join(sorted(recursions)))
    if unknown_syms:
        warnings.append('未解析调用目标: %s' % ', '.join(sorted(unknown_syms)))
    seen = set()
    uniq_warnings = []
    for w in warnings:
        if w not in seen:
            seen.add(w)
            uniq_warnings.append(w)
    if uniq_warnings:
        out.append('')
        out.append('警告:')
        for w in uniq_warnings:
            out.append('  - ' + w)

    if not args.quiet:
        print('\n'.join(out))

    if args.stack_size is not None and worst > args.stack_size:
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
