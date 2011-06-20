import Options
import Utils
import platform

APPNAME='webdavui'
VERSION='0.1.0'

#-- OPTIONS --
def options(opt):
	opt.load('compiler_cxx')
	opt.add_option('--libreoffice-prefix',
	               action='store',
	               default='/usr/lib/libreoffice',
	               dest="LO_PREFIX",
	               help='Libreoffice prefix')
   	opt.add_option('--ure-prefix',
	               action='store',
	               default='/usr/lib/ure',
	               dest="URE_PREFIX",
	               help='Libreoffice prefix')

#-- CONFIGURE TARGET --
def configure(conf):
	conf.load('compiler_cxx')
	
	uno_sdk_libpath = '%s/basis3.3/sdk/lib' % Options.options.LO_PREFIX

	conf.check_cxx(lib='uno_sal', uselib_store='SALLIB', libpath=uno_sdk_libpath, mandatory=True)
	conf.check_cxx(lib='uno_cppu', uselib_store='CPPULIB', libpath=uno_sdk_libpath, mandatory=True)
	conf.check_cxx(lib='uno_cppuhelpergcc3', uselib_store='CPPUHELPERLIB', libpath=uno_sdk_libpath,  mandatory=True)

	conf.find_program('cppumaker', var='CPPUMAKER', path_list=[Options.options.LO_PREFIX + '/basis3.3/sdk/bin'], mandatory=True)

	conf.env['TYPES_RDB'] = conf.find_file('types.rdb',
	                                       path_list=[Options.options.URE_PREFIX + '/share/misc/'],
	                                       mandatory=True)

	conf.env['OFFAPI_RDB'] = conf.find_file('offapi.rdb',
	                                        path_list=[Options.options.LO_PREFIX + '/basis3.3/program'],
	                                        mandatory=True)

def cppumaker (bld):
	"""
	This helper generates the C++ headers for the IDL of the classes used.
	These headers are stored inside the build/lo/include directory.
	"""
	types = ["com.sun.star.awt.WindowAttribute",
				"com.sun.star.awt.Key",
				"com.sun.star.awt.KeyEvent",
				"com.sun.star.awt.MenuEvent",
				"com.sun.star.awt.KeyModifier",
				"com.sun.star.awt.XActionListener",
				"com.sun.star.awt.XButton",
				"com.sun.star.awt.XControl",
				"com.sun.star.awt.XControlContainer",
				"com.sun.star.awt.XControlModel",
				"com.sun.star.awt.XDialog",
				"com.sun.star.awt.XItemList",
				"com.sun.star.awt.XMessageBox",
				"com.sun.star.awt.XWindow2",
				"com.sun.star.awt.XWindowPeer",
				"com.sun.star.awt.XMenu",
				"com.sun.star.awt.XMenuBar",
				"com.sun.star.awt.XMenuExtended",
				"com.sun.star.awt.XPopupMenu",
				"com.sun.star.awt.XPopupMenuExtended",
				"com.sun.star.beans.NamedValue",
				"com.sun.star.beans.PropertyValue",
				"com.sun.star.beans.XPropertySet",
				"com.sun.star.container.XContainerQuery",
				"com.sun.star.container.XIndexContainer",
				"com.sun.star.container.XIndexAccess",
				"com.sun.star.container.XNameAccess",
				"com.sun.star.container.XNameContainer",
				"com.sun.star.container.NoSuchElementException",
				"com.sun.star.frame.ControlCommand",
				"com.sun.star.frame.ControlEvent",
				"com.sun.star.frame.DispatchDescriptor",
				"com.sun.star.frame.XComponentLoader",
				"com.sun.star.frame.XController",
				"com.sun.star.frame.XControlNotificationListener",
				"com.sun.star.frame.XDispatch",
				"com.sun.star.frame.XDispatchProvider",
				"com.sun.star.frame.XDispatchProviderInterception",
				"com.sun.star.frame.XDispatchHelper",
				"com.sun.star.frame.XFrame",
				"com.sun.star.frame.XFrameActionListener",
				"com.sun.star.frame.FrameActionEvent",
				"com.sun.star.frame.XLayoutManager",
				"com.sun.star.frame.XModel",
				"com.sun.star.frame.XStorable",
				"com.sun.star.frame.XStatusListener",
				"com.sun.star.frame.XModuleManager",
				"com.sun.star.frame.XStatusListener",
				"com.sun.star.frame.XPopupMenuController",
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
				"com.sun.star.uno.XAggregation",
				"com.sun.star.uno.XComponentContext",
				"com.sun.star.uno.XWeak",
				"com.sun.star.util.URL",
				"com.sun.star.util.XURLTransformer",
				"com.sun.star.task.XJob"]
	
	types_db = '%s %s' % (bld.env.TYPES_RDB, bld.env.OFFAPI_RDB)

	types_str = "-T" + " -T".join(types)
	target = bld.path.get_bld().abspath() + '/lo/include'

	bld.exec_command ('%s -Gc -BUCR -O%s %s %s' % (bld.env.CPPUMAKER, target, types_str, types_db))

	return target

def build(bld):
	includes = ['/usr/include/libreoffice']
	
	includes.append (cppumaker (bld))

	print includes
	
	env = bld.env.copy()
	pattern = env['cxxshlib_PATTERN']
	env['cxxshlib_PATTERN']	= '%s.uno' + env['cxxshlib_PATTERN'].split("%s")[-1]
	env.append_value('LINKFLAGS', '-Wl,--version-script=%s/data/component.uno.map,-z origin' % bld.path.abspath())
	env.append_value('CXXFLAGS', '-Wall')
	env.append_value('CXXFLAGS', '-g')

	target = 'webdavui'

	bits = platform.architecture()[0][:-3]

	if bits == "64":
		lo_platform = "Linux_x86_64"
	else:
		lo_platform = "Linux_x86"

	bld.shlib(source=['src/component.cxx',
			  'src/addon.cxx',
			  'src/webdavdialog.cxx'],
	          target=target,
	          uselib=['SALLIB', 'CPPULIB', 'CPPUHELPERLIB' ],
	          includes=includes,
	          defines=['UNX', 'SAL_UNX', 'CPPU_ENV=gcc3'],
	          install_path='%s/share/extensions/%s/%s' % (Options.options.LO_PREFIX, target, lo_platform),
	          env=env,
	          chmod=Utils.O644)

	bld(features='subst',
	    source='data/manifest.xml.in',
	    target='manifest.xml',
	    install_path='%s/share/extensions/%s/META-INF' % (Options.options.LO_PREFIX, target),
	    PLATFORM=lo_platform,
	    COMPONENT=env['cxxshlib_PATTERN'] % target)

	bld.install_files('%s/share/extensions/%s' % (Options.options.LO_PREFIX, target), bld.path.ant_glob('data/*.xcu'))
	bld.install_files('%s/share/extensions/%s/images' % (Options.options.LO_PREFIX, target), bld.path.ant_glob('data/images/*.png'))
	bld.install_files('%s/share/extensions/%s' % (Options.options.LO_PREFIX, target), bld.path.ant_glob('data/*.txt'))

