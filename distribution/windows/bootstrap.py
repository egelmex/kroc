#!/bin/python

import sys
import os
import os.path
import subprocess
import tarfile
import zipfile
import urllib
import urllib2
import fnmatch
import shutil
import re

paths = dict(
  downloads = 'downloads',
  mingw     = 'mingw',
  msys      = 'msys',
  tmp       = 'tmp',
)

class UrlMapping(dict):
    def url_list(self):
        return self.values()
    def file_list(self):
        values = self.values()
        return [os.path.join(paths['downloads'], v.split('/')[-1]) for v in values]
    def url_dict(self):
        return dict(self.items())
    def file_dict(self):
        items = self.items()
        d = dict()
        for k, v in items:
            d[k] = os.path.join(paths['downloads'], v.split('/')[-1])
        return d
   
# Check that there are no spaces in the name of the current path, that would
# be bad...
if os.getcwd().count(' ') > 0:
    name = os.path.basename(__file__)
    sys.stdout.write('Please run %s from a directory without spaces\n' % (name,))
    sys.stdout.write('You ran %s from "%s"\n' % (name, os.getcwd()))
    sys.exit(1)

def find_file(name, path, ext=''):
    """Given a pathsep-delimited path string, find name,.
    Returns path to name, if found, otherwise None.
    Also allows for files with implicit extensions (eg, .exe), but
    always returning name, as was provided.
    >>> find_file('ls', '/usr/bin:/bin', ext='.exe')
    '/bin/ls'
    """
    if os.path.isfile(name):
        return name
    if ext and os.path.isfile(name + ext):
        # Already absolute path.
        return name + ext
    for p in path.split(os.pathsep):
        candidate = os.path.join(p, name)
        if (os.path.isfile(candidate)):
            return candidate
        if ext and os.path.isfile(candidate + ext):
            return candidate + ext
    return None


def download(url, dest = None):
    sys.stdout.write('%s\n' % (url, ))
    if dest != None:
        f = urllib2.urlopen(url)
        expected_size = f.info().getheader('Content-Length')
        try:
            file_size    = os.path.getsize(dest)
            if int(expected_size) == int(file_size):
                sys.stdout.write('  already downloaded, skipping...\n')   
                return None
        except WindowsError:
            pass
    progress_printed = [0]
    def progress(block, block_size, total_size):
        progress = min(((block * block_size * 100)/total_size), 100)
        progress = int(progress * 0.70)
        if progress > progress_printed[0]:
            sys.stdout.write('#' * (progress - progress_printed[0]))
            progress_printed[0] += progress - progress_printed[0]
    if sys.stdout.isatty():
        sys.stdout.write('  !' + (' ' * 33) + '50%' + (' ' * 32) + '! 100%\n')
        sys.stdout.write('  ')
        u = urllib.urlretrieve(url, dest, progress)
        sys.stdout.write('\n')
        return u[0]
    else:
        u = urllib.urlretrieve(url, dest)
        return u[0]

def makedir(dir):
    if os.path.exists(dir):
        if not os.path.isdir(dir):
            sys.stdout.write('Existing file "%s" is not a directory, plase delete\n' % dir)
            sys.exit(1)
    else:
        os.makedirs(dir)

def extract(files, dest):
    for f in files:
        sys.stdout.write('extracting: %s\n' %(f, ))
        if f.endswith('.lzma'):
            lzma = subprocess.Popen([os.path.join(paths['msys'], 'bin', 'lzma'), '-d', '-c', '-q', f], stdout=subprocess.PIPE)
            a = tarfile.open(fileobj=lzma.stdout, mode='r|')
            # FIXME: check that no files will be written outside dest
            a.extractall(dest)
            a.close()
            r = lzma.wait()
            if r != 0:
                sys.stdout.write('lzma failed with error code: %d %s\n' %(r, f))
        elif f.endswith('.zip'):
            a = zipfile.ZipFile(f, 'r')
            # extractall is broken for zipfile...
            names = a.namelist()
            for name in names:
                if name[-1] == '/':
                    pass
                else:
                    a.extract(name, dest)
            a.close()
        else:
            a = tarfile.open(f, 'r')
            # FIXME: check that no files will be written outside dest
            a.extractall(dest)
            a.close()
        
def read_urls(file_name, mapping=False):
    fp = open(file_name, 'r')
    lines = process_lines(fp.readlines())
    fp.close()
    if mapping:
        mapping = UrlMapping()
        for line in lines:
            k, v = line.split(':', 1)
            mapping[k.strip()] = v.strip()
        return mapping
    return lines

def process_lines(lines):
    return [x for x in [x.strip() for x in lines] if not (x.startswith('#') or len(x) == 0)]

mingw_urls = read_urls('mingw.urls')
msys_urls = read_urls('msys.urls')
perl_urls = read_urls('perl.urls', mapping=True)

mingw_files = []
msys_files = []
for url in mingw_urls:
    mingw_files.append(os.path.join(paths['downloads'], url.split('/')[-1]))
for url in msys_urls:
    msys_files.append(os.path.join(paths['downloads'], url.split('/')[-1]))

urls  = mingw_urls + msys_urls + perl_urls.url_list()
files = mingw_files + msys_files + perl_urls.file_list()

#urls  = msys_urls; files = msys_files
#urls = []

makedir(paths['downloads'])
for url, dest in zip(urls, files):
    url = url.strip()
    if url.startswith('#'): continue
    if len(url) == 0: continue
    download(url, dest)
 
makedir(paths['mingw'])
makedir(paths['msys'])
makedir(paths['tmp'])

extract(msys_files, paths['msys'])
extract(mingw_files, paths['mingw'])
extract(perl_urls.file_list(), paths['tmp'])

print 'creating fstab'
tvmdir = os.path.abspath(os.path.join(os.getcwd(), '../../')).replace('\\', '/')
msysdir = os.path.abspath(paths['mingw']).replace('\\', '/')
fp = open('msys/etc/fstab', 'w')
fp.write('%s\t%s\n' % (tvmdir, '/tvm'))
fp.write('%s\t%s\n' % (msysdir, '/mingw'))
fp.close()

print 'setting extra msys config'
cfg_dir     = os.path.join(paths['msys'], 'etc', 'profile.d')
cfg_file    = os.path.join(cfg_dir, 'tvm_config.sh')
perl_dir    = '/perl/bin'
install_dir = '/tvm/distribution/windows/install/bin'
path_var    = [perl_dir, install_dir]
if not os.path.exists(cfg_dir):
	os.makedirs(cfg_dir)
fp = open(cfg_file, 'w')
fp.write('# tvm related config options for the msys shell\n')
fp.write('# created automagically by bootstrap.py\n')
fp.write('export PATH=.:%s:$PATH\n' % ':'.join(path_var))
fp.write('export EDITOR=vim\n')
fp.close()

print 'setting up java path script'
cfg_file    = os.path.join(cfg_dir, 'java_path.sh')
fp = open(cfg_file, 'w')
fp.write(r"""
JDK_REG_ROOT="HKLM\\SOFTWARE\\JavaSoft\\Java Development Kit"

javac=`which javac 2>/dev/null`

if test "x$javac" != x ; then
	echo "Javac found in $javac, not altering path"
	return
fi

version=`reg query "$JDK_REG_ROOT" //v CurrentVersion 2>/dev/null | grep CurrentVersion | gawk "{print \\$3}"`
home=`reg query "$JDK_REG_ROOT\\\\$version" //v JavaHome 2>/dev/null | grep JavaHome | gawk "{ print substr(\\$0, index(\\$0, \\$3)) }"`

if test "x$home" != x ; then
	echo "JDK version $version found, in $home"
	path=`echo $home | sed -e "s/:// ; s|\\\\\|/|g"`
	path="/$path/bin"
	echo "Setting path to include $path"
	export PATH="$PATH":"$path"
	echo "Setting JAVA_HOME to include $home"
	export JAVA_HOME="$home"
	return
fi

echo "Javac not found and could not find a JDK through the registry"
echo "Please make sure you have an appropritate java version installed"
""")
fp.close()

print 'renaming autotools'
r = re.compile('(aclocal|auto.*?)-.*')
bindir = os.path.join(paths['mingw'], 'bin')
autotools = os.listdir(bindir)
autotools = [at for at in autotools if r.match(at)]
tools = [t[:t.index('-')] for t in autotools]
for tool in tools:
    versions = fnmatch.filter(autotools, tool + '-*')
    versions.sort()
    shutil.copy(os.path.join(bindir, versions[0]),
                os.path.join(bindir, tool))

build_dir   = os.path.join(
                  paths['tmp'], 
                  perl_urls.file_dict()['perl'][len('downloads/'):-len('.tar.bz2')],
                  'win32')
dmake_dir   = os.path.join(os.getcwd(), paths['tmp'], 'dmake')
mingw_dir   = os.path.join(os.getcwd(), paths['mingw'])
msys_dir    = os.path.join(os.getcwd(), paths['msys'])
mingw_bin   = os.path.join(mingw_dir, 'bin')
env         = dict(os.environ)
env['PATH'] = ';'.join([env['PATH'], dmake_dir, mingw_dir, mingw_bin])
dmake       = find_file('dmake', env['PATH'], '.exe')
inst_dir    = os.path.join(msys_dir, 'perl')

print 'patching perl makefile'
re1 = re.compile(r'^\s*INST_DRV\s+\*=.*', re.MULTILINE)
re2 = re.compile(r'^\s*INST_TOP\s+\*=.*', re.MULTILINE)
re3 = re.compile(r'^\s*CCHOME\s+\*=.*', re.MULTILINE)
drive, path = os.path.splitdrive(inst_dir)
fp = open(os.path.join(build_dir, 'makefile.mk'), 'r+')
text = fp.read()
text = re1.sub('INST_DRV *= ' + drive, text)
text = re2.sub('INST_TOP *= $(INST_DRV)' + path.replace('\\', '\\\\'), text)
text = re3.sub('CCHOME *= ' + mingw_dir.replace('\\', '\\\\'), text)
fp.seek(0)
fp.truncate()
fp.write(text)
fp.close()

print 'compiling perl'
try:
    retcode = subprocess.call(dmake, cwd=build_dir, env=env)
    if retcode < 0:
        print >>sys.stderr, "Child was terminated by signal", -retcode
    elif retcode != 0:
        print >>sys.stderr, "Child returned", retcode
except OSError, e:
    print >>sys.stderr, "Execution failed:", e

print 'installing perl'
try:
    retcode = subprocess.call([dmake, 'install'], cwd=build_dir, env=env)
    if retcode < 0:
        print >>sys.stderr, "Child was terminated by signal", -retcode
    elif retcode != 0:
        print >>sys.stderr, "Child returned", retcode
except OSError, e:
    print >>sys.stderr, "Execution failed:", e


