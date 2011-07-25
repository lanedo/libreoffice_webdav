import Options
import Utils
import platform
import shutil
import os
import glob
import sys

APPNAME='webdavui'
VERSION='1.0.0'

data_file_patterns = ['data/*.xcu', 'data/*.xcs', 'data/*.txt', 'data/*.xdl', 'data/*.default', 'data/*.properties']
image_file_patterns = 'data/images/*.png'

if Options.platform in ('cygwin', 'win32'):
    lo_setsdkenv = 'C:/Apps/LibreOffice3.4/Basis/sdk/setsdkenv_windows.bat'
    try:
        default_include_prefix = '%s/include' % os.environ ('OO_SDK_HOME')
    except:
        default_include_prefix = 'C:/Apps/LibreOffice3.4/Basis/sdk/include'
    uno_sal = 'isal'
    uno_cppu = 'icppu'
    uno_cppuhelpergcc3 = 'icppuhelper'
    lo_platform = 'Windows'
    lo_platform_defines = 'WIN32 WNT'
else:
    lo_setsdkenv = '/usr/lib/libreoffice/basis-link/sdk/setsdkenv_unix'
    default_include_prefix = '/usr/include/libreoffice'
    uno_sal = 'uno_sal'
    uno_cppu = 'uno_cppu'
    uno_cppuhelpergcc3 = 'uno_cppuhelpergcc3'
    if platform.architecture()[0][:-3] == '64':
        lo_platform = 'Linux_x86_64'
    else:
        lo_platform = 'Linux_x86'
    lo_platform_defines = 'UNX SAL_UNX'

#-- OPTIONS --
def options(opt):
	opt.load('compiler_cxx')
	opt.get_option_group ('--check-cxx-compiler').add_option('-d', '--debug',
		action = 'store_true', default = False,
		help = 'Enable debugging and verbose output.', dest = 'enable_debug')

#-- CONFIGURE TARGET --
def configure(conf):
	conf.load('compiler_cxx')
	if Options.options.enable_debug:
		conf.env.append_value ('CXXFLAGS', '-DDEBUG')

	try:
		for envvar in ['OO_SDK_HOME  ', 'OFFICE_PROGRAM_PATH', 'OFFICE_BASE_PROGRAM_PATH']:
			print '  %s:\t\t%s' % (envvar, os.environ[envvar.strip ()])
	except Exception:
		msg = sys.exc_info()[1] # Python 2/3 compatibility
		print 'Variable %s not set!' % msg
		print 'The LibreOffice SDK environment is not set!'
		print 'You need to run %s or similar' % lo_setsdkenv
		sys.exit (1)
	uno_sdk_libpath = os.environ['OO_SDK_HOME'] + '/lib'
	conf.env ['OFFICE_HOME'] = os.environ['OFFICE_BASE_PROGRAM_PATH'][:-8]

	conf.check_cxx(lib=uno_sal, uselib_store='SALLIB', libpath=uno_sdk_libpath, mandatory=True)
	conf.check_cxx(lib=uno_cppu, uselib_store='CPPULIB', libpath=uno_sdk_libpath, mandatory=True)
	conf.check_cxx(lib=uno_cppuhelpergcc3, uselib_store='CPPUHELPERLIB', libpath=uno_sdk_libpath,  mandatory=True)

	conf.find_program('cppumaker', var='CPPUMAKER', \
	    path_list=[os.environ['OO_SDK_HOME'] + '/bin'], mandatory=True)
	conf.env['TYPES_RDB'] = conf.find_file('types.rdb',
	    path_list=[os.environ['OO_SDK_URE_HOME'] + '/share/misc/', \
	    os.environ['OO_SDK_URE_HOME'] + '/misc/'], mandatory=True)
	conf.env['OFFAPI_RDB'] = conf.find_file('offapi.rdb',
	    path_list=[os.environ['OFFICE_BASE_PROGRAM_PATH']], mandatory=True)

def cppumaker (bld):
	"Generate the C++ headers for the IDL."
	types = ["com.sun.star.awt.WindowAttribute",
				"com.sun.star.awt.Key",
				"com.sun.star.awt.KeyEvent",
				"com.sun.star.awt.KeyModifier",
				"com.sun.star.awt.MenuEvent",
				"com.sun.star.awt.PosSize",
				"com.sun.star.awt.XActionListener",
				"com.sun.star.awt.XButton",
				"com.sun.star.awt.XControl",
				"com.sun.star.awt.XControlContainer",
				"com.sun.star.awt.XControlModel",
				"com.sun.star.awt.XDialog",
				"com.sun.star.awt.XDialogProvider2",
				"com.sun.star.awt.XItemList",
				"com.sun.star.awt.XItemListener",
				"com.sun.star.awt.XMenu",
				"com.sun.star.awt.XMenuBar",
				"com.sun.star.awt.XMenuExtended",
				"com.sun.star.awt.XMessageBox",
				"com.sun.star.awt.MessageBoxButtons",
				"com.sun.star.awt.XMessageBoxFactory",
				"com.sun.star.awt.XListBox",
				"com.sun.star.awt.XPopupMenu",
				"com.sun.star.awt.XPopupMenuExtended",
				"com.sun.star.awt.XWindow2",
				"com.sun.star.awt.XWindowPeer",
				"com.sun.star.beans.NamedValue",
				"com.sun.star.beans.PropertyValue",
				"com.sun.star.beans.XPropertySet",
				"com.sun.star.container.XContainerQuery",
				"com.sun.star.container.XIndexAccess",
				"com.sun.star.container.XIndexContainer",
				"com.sun.star.container.XNameAccess",
				"com.sun.star.container.XNameContainer",
				"com.sun.star.container.NoSuchElementException",
				"com.sun.star.deployment.PackageInformationProvider",
				"com.sun.star.deployment.XPackageInformationProvider",
				"com.sun.star.frame.ControlCommand",
				"com.sun.star.frame.ControlEvent",
				"com.sun.star.frame.DispatchDescriptor",
				"com.sun.star.frame.FrameActionEvent",
				"com.sun.star.frame.FrameSearchFlag",
				"com.sun.star.frame.XComponentLoader",
				"com.sun.star.frame.XController",
				"com.sun.star.frame.XControlNotificationListener",
				"com.sun.star.frame.XDispatch",
				"com.sun.star.frame.XDispatchHelper",
				"com.sun.star.frame.XDispatchProvider",
				"com.sun.star.frame.XDispatchProviderInterception",
				"com.sun.star.frame.XFrame",
				"com.sun.star.frame.XFrameActionListener",
				"com.sun.star.frame.XLayoutManager",
				"com.sun.star.frame.XModel",
				"com.sun.star.frame.XModuleManager",
				"com.sun.star.frame.XPopupMenuController",
				"com.sun.star.frame.XStatusListener",
				"com.sun.star.frame.XStorable",
				"com.sun.star.frame.status.Visibility",
				"com.sun.star.lang.EventObject",
				"com.sun.star.lang.XComponent",
				"com.sun.star.lang.XInitialization",
				"com.sun.star.lang.XMultiServiceFactory",
				"com.sun.star.lang.XServiceInfo",
				"com.sun.star.lang.XSingleComponentFactory",
				"com.sun.star.lang.XSingleServiceFactory",
				"com.sun.star.lang.XTypeProvider",
				"com.sun.star.registry.XRegistryKey",
				"com.sun.star.ucb.XSimpleFileAccess",
				"com.sun.star.uno.DeploymentException",
				"com.sun.star.uno.XAggregation",
				"com.sun.star.uno.XComponentContext",
				"com.sun.star.uno.XWeak",
				"com.sun.star.util.URL",
				"com.sun.star.util.XChangesBatch",
				"com.sun.star.util.XURLTransformer",
				"com.sun.star.task.XJob"]
	
	types_db = '%s %s' % (bld.env.TYPES_RDB, bld.env.OFFAPI_RDB)

	types_str = "-T" + " -T".join(types)
	target = bld.path.get_bld().abspath() + '/lo/include'

	bld.exec_command ('%s -Gc -BUCR -O%s %s %s' % (bld.env.CPPUMAKER, target, types_str, types_db))

	return target

def build(bld):
	includes = [default_include_prefix, cppumaker (bld)]

	target = 'webdavui'
	env = bld.env.copy()
	pattern = env['cxxshlib_PATTERN']
	env['cxxshlib_PATTERN']	= '%s.uno' + env['cxxshlib_PATTERN'].split("%s")[-1]
	if bld.env['CC_NAME'] == 'msvc':
		env.append_value('CXXFLAGS', (''
			+ '-Zm500 -Zc:forScope,wchar_t- -wd4251 -wd4275 -wd4290 '
			+ '-wd4675 -wd4786 -wd4800 -Zc:forScope -GR '
			+ '/DEBUGTYPE:cv -MT /EHa -DCPPU_ENV=msci '
			+ '').split (' '))
		env.append_value('LINKFLAGS', '/DEF:%s/data/component.uno.def' % bld.path.abspath())
	else:
		env.append_value('CXXFLAGS', '-g -Wall -DCPPU_ENV=gcc3'.split(' '))
		env.append_value('LINKFLAGS', \
			'-Wl,--version-script=%s/data/component.uno.map,-z origin' % bld.path.abspath())

	office_home = bld.env['OFFICE_HOME']
	bld.shlib(source=['src/component.cxx',
			  'src/addon.cxx',
			  'src/filedialog.cxx',
			  'src/settings.cxx',
			  'src/configdialog.cxx'],
	          target=target,
	          uselib=['SALLIB', 'CPPULIB', 'CPPUHELPERLIB' ],
	          includes=includes,
	          defines=lo_platform_defines.split (' '),
	          install_path='%s/share/extensions/%s/%s' % \
	              (office_home, target, lo_platform),
	          env=env,
	          chmod=Utils.O644)

	bld(features='subst',
	    source='data/manifest.xml.in',
	    target='manifest.xml',
	    install_path='%s/share/extensions/%s/META-INF' % (office_home, target),
	    PLATFORM=lo_platform,
	    COMPONENT=env['cxxshlib_PATTERN'] % target)

	for pattern in data_file_patterns:
		bld.install_files('%s/share/extensions/%s' % \
		(office_home, target), bld.path.ant_glob(pattern))
	bld.install_files('%s/share/extensions/%s/images' % \
	    (office_home, target), bld.path.ant_glob(image_file_patterns))

def mkdir (folder):
    if not os.path.exists (folder):
        os.mkdir (folder)

def oxt(a):
	"Generate a self-contained OXT extension bundle."
	cwd = os.getcwd () + '/'
	mkdir (cwd + 'dist')
	mkdir (cwd + 'dist/images')
	for pattern in data_file_patterns:
		for filename in glob.iglob (pattern):
			shutil.copy2 (filename, cwd + 'dist')
	for filename in glob.iglob (image_file_patterns):
		shutil.copy2 (filename, cwd + 'dist/images')

	shutil.copy2 (cwd + 'description.xml', cwd + 'dist')
	mkdir (cwd + 'dist/META-INF')
	if not os.path.exists (cwd + 'build/manifest.xml'):
		print 'Run ./waf build first!'
		sys.exit (1)
	shutil.copy2 (cwd + 'build/manifest.xml', cwd + 'dist/META-INF')
	mkdir (cwd + 'dist/' + lo_platform)
	for filename in glob.iglob ('build/webdavui.uno.*'):
		if filename[-9:] == '.manifest':
			continue
		shutil.copy2 (filename , cwd + 'dist/' + lo_platform)
		if lo_platform == 'Windows':
			manifesto = 'mt -nologo -manifest %s.manifest "-outputresource:%s;2"' \
			% (cwd + 'build/webdavui.uno.dll', cwd + 'dist/Windows/webdavui.uno.dll')
			os.system (manifesto)
	try:
		shutil.make_archive (cwd + 'webdavui', 'zip', cwd + 'dist')
		shutil.move (cwd + 'webdavui.zip', cwd + 'webdavui.oxt')
	except:
		# Lame Unix-only < Python 2.7 fallback
		os.system ('rm -f webdavui.oxt && cd dist && zip -r ../webdavui.oxt .')

