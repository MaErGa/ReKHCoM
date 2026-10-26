"""Create named RCDATA entries for reviewed core assets; no game dumps or HD imports."""
import argparse
from pathlib import Path
from core_assets import ROOT, core_assets


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--out', type=Path, required=True)
    args = p.parse_args()
    lines = ['// Generated core resources. Fonts are in rekhcom.rc.']
    keys = set()
    for name, source in sorted(core_assets().items()):
        if not source.is_file():
            p.error(f'missing core asset: {source}')
        key = 14695981039346656037
        for byte in name.encode('utf-8'):
            key = ((key ^ byte) * 1099511628211) & 0xffffffffffffffff
        if key in keys:
            p.error(f'core resource key collision: {name}')
        keys.add(key)
        lines.append(f'CORE_ASSET_{key:016X} RCDATA "{source.relative_to(ROOT).as_posix()}"')
    args.out.parent.mkdir(parents=True, exist_ok=True)
    text = '\n'.join(lines) + '\n'
    if not args.out.exists() or args.out.read_text(encoding='utf-8') != text:
        args.out.write_text(text, encoding='utf-8')


if __name__ == '__main__':
    main()
