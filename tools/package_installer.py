"""Create a native installer with natID SetupCollector (run on each target OS)."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from xml.sax.saxutils import escape

parser = argparse.ArgumentParser()
parser.add_argument('--sdk', type=Path, required=True, help='Matching natID.SDK root')
parser.add_argument('--collector', type=Path, required=True, help='Native SetupCollector executable')
parser.add_argument('--executable-dir', type=Path, required=True, help='Release output directory containing obidji-jugu executable/app')
parser.add_argument('--license', type=Path, required=True, help='Project distribution license approved by the authors')
parser.add_argument('--output', type=Path, default=Path('installer-output'))
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
for path in (args.sdk, args.collector, args.executable_dir, args.license):
    if not path.exists():
        parser.error(f'Missing: {path}')
if not any(args.executable_dir.glob('obidji-jugu*')):
    parser.error('Release output must contain obidji-jugu, obidji-jugu.exe or obidji-jugu.app')
subprocess.run([sys.executable, str(root / 'tools/generate_icons.py')], check=True)
args.output = args.output.resolve()
args.output.mkdir(parents=True, exist_ok=True)
# Unique, retained staging folder makes failures inspectable and avoids deleting
# another package/run. Neither SDK headers, binaries nor package XML are edited.
stage = Path(tempfile.mkdtemp(prefix='obidji-jugu-package-'))
source = stage / 'source'
shutil.copytree(root / 'res', source / 'res')
configs = stage / 'collectors'
shutil.copytree(args.sdk / 'DevEnv/SetupCollectors', configs)

# The old development layout uses ~/Work; packaged resources resolve against
# the actual SDK, on macOS/Linux as well as Windows.
for xml in (source / 'res').rglob('*.xml'):
    content = xml.read_text(encoding='utf-8-sig')
    content = content.replace('Work/Common/', args.sdk.resolve().as_posix() + '/Common/')
    xml.write_text(content, encoding='utf-8')
template = (root / 'packaging/ObidjiJugu.xml').read_text(encoding='utf-8')
for token, path in {'SOURCE':source, 'EXECUTABLE_DIR':args.executable_dir.resolve(),
                    'OUTPUT':args.output / 'obidji-jugu', 'LICENSE':args.license.resolve()}.items():
    template = template.replace('@' + token + '@', escape(path.as_posix(), {'"':'&quot;'}))
config = configs / 'ObidjiJugu.xml'
config.write_text(template, encoding='utf-8')
subprocess.run([str(args.collector.resolve()), str(config)], cwd=stage, check=True)
print(f'Installer output: {args.output}')
print(f'Collector staging (retained): {stage}')
