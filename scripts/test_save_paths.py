"""Exercise production save roles and process restart in ASCII and Chinese paths."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', required=True, type=Path)
    parser.add_argument('--evidence-dir', required=True, type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    evidence = args.evidence_dir.resolve()
    evidence.mkdir(parents=True, exist_ok=False)
    save_input = evidence / 'save.txt'
    save_input.write_text('10 F5 down\n11 F5 up\n', encoding='utf-8')
    load_input = evidence / 'load.txt'
    load_input.write_text('2 F9 down\n3 F9 up\n', encoding='utf-8')
    steps = []
    for label in ('ascii-user', '测试用户数据'):
        user = evidence / label / 'WRITEOVER07'
        user.mkdir(parents=True)

        def run(phase, replay, frames, expected):
            result = subprocess.run([
                str(args.executable.resolve()), '--replay', str(replay),
                '--frames', str(frames), '--width', '80', '--height', '30',
                '--data-dir', str(root / 'data'), '--user-data-dir', '.'
            ], cwd=user, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
            (evidence / (label + '-' + phase + '.log')).write_bytes(result.stdout)
            text = result.stdout.decode('utf-8', errors='replace')
            passed = result.returncode == 0 and all(item in text for item in expected)
            steps.append({'case': label, 'phase': phase, 'passed': passed})
            if not passed:
                raise RuntimeError(label + ' ' + phase + ' failed; see evidence log')

        def snapshot():
            return {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                    for p in (user / 'saves').glob('*.wo07')}

        run('save', save_input, 35, ['SAVE_ATTEMPTED=YES SAVE_OK=YES'])
        assert set(snapshot()) == {'pvs_manual.wo07', 'pvs_resume.wo07'}
        run('restart', load_input, 35, ['LOAD_ATTEMPTED=YES LOAD_OK=YES'])
        run('replace', save_input, 55, ['SAVE_ATTEMPTED=YES SAVE_OK=YES'])
        run('restart-after-replace', load_input, 35, ['LOAD_ATTEMPTED=YES LOAD_OK=YES'])
        before = snapshot()
        run('new-game', root / 'tools/replay/product_new_game.txt', 80,
            ['PRODUCT_NEW_GAME_INITIAL_STATE=PASS', 'ROOM=room_b1_revival HEALTH=100 EVIDENCE=0'])
        assert snapshot() == before, 'New Game changed previous saves'
        assert not list((user / 'saves').glob('*.tmp'))
        print('SAVE_PATH_CASE=' + label + ' PASS', flush=True)
    (evidence / 'result.json').write_text(json.dumps(
        {'status': 'PASS', 'steps': steps}, ensure_ascii=False, indent=2), encoding='utf-8')
    print('SAVE_PATH_REGRESSION=PASS')


if __name__ == '__main__':
    main()
