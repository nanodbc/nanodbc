# Build the documentation for nanodbc library

# Configuration
nanodbc_name = 'nanodbc'
nanodbc_versions = ['master', '2.12.4']
# End of Configuration

import errno
import os
import sys
from subprocess import check_call, CalledProcessError, Popen, PIPE

def build_docs(**kwargs):
    assert nanodbc_versions
    version = kwargs.get('version', nanodbc_versions[0])
    doc_dir = kwargs.get('doc_dir', os.path.dirname(
        os.path.realpath(__file__)))
    work_dir = kwargs.get('work_dir', '.')
    include_dir = kwargs.get('include_dir', os.path.join(
        os.path.dirname(doc_dir), 'nanodbc'))
    doxyxml_dir = os.path.join(work_dir, 'doxyxml')

    doxyfile = r'''
        PROJECT_NAME      = {0}
        GENERATE_XML      = YES
        GENERATE_HTML     = NO
        GENERATE_LATEX    = NO
        INPUT             = {1}
        JAVADOC_AUTOBRIEF = YES
        AUTOLINK_SUPPORT  = NO
        XML_OUTPUT        = {2}
        MACRO_EXPANSION   = YES
        PREDEFINED        = DOXYGEN=1
        '''.format(nanodbc_name, include_dir, doxyxml_dir).encode('UTF-8')
    cmd = ['doxygen', '-']
    p = Popen(cmd, stdin=PIPE)
    p.communicate(input=doxyfile)
    if p.returncode != 0:
        raise CalledProcessError(p.returncode, cmd)
    sys.exit()
    html_dir = os.path.join(work_dir, 'html')
    versions = nanodbc_versions
    assert versions
    check_call(['sphinx-build',
                '-Dbreathe_projects.format=' + os.path.abspath(doxyxml_dir),
                '-Dversion=' + version, '-Drelease=' + version,
                '-Aversion=' + version, '-Aversions=' + ','.join(versions),
                '-b', 'html', doc_dir, html_dir])
    try:
        check_call(['lessc', '--clean-css',
                    '--include-path=' + os.path.join(doc_dir, 'bootstrap'),
                    os.path.join(doc_dir, 'nanodbc.less'),
                    os.path.join(html_dir, '_static', 'nanodbc.css')])
    except OSError as err:
        if err.errno != errno.ENOENT:
            raise
        print('lessc (http://lesscss.org/) not found')
        sys.exit(1)
    return html_dir

if __name__ == '__main__':
    #create_build_env()
    build_docs()
