project(
     'gnome-todo',
              'c',
          version: '3.27.2',
          license: 'GPL3+',
  default_options: [ 'buildtype=debugoptimized', 'warning_level=1' ],
    meson_version: '>= 0.41.0'
)

gnome_todo_version = meson.project_version()
version_array = gnome_todo_version.split('.')
gnome_todo_major_version = version_array[0].to_int()
gnome_todo_minor_version = version_array[1].to_int()
gnome_todo_micro_version = version_array[2].to_int()

gnome_todo_gir_namespace = 'Gtd'
gnome_todo_gir_version = '1.0'

gnome_todo_prefix = get_option('prefix')
gnome_todo_bindir = join_paths(gnome_todo_prefix, get_option('bindir'))
gnome_todo_datadir = join_paths(gnome_todo_prefix, get_option('datadir'))
gnome_todo_includedir = join_paths(gnome_todo_prefix, get_option('includedir'))
gnome_todo_libdir = join_paths(gnome_todo_prefix, get_option('libdir'))
gnome_todo_libexecdir = join_paths(gnome_todo_prefix, get_option('libexecdir'))
gnome_todo_localedir = join_paths(gnome_todo_prefix, get_option('localedir'))

gnome_todo_pkgdatadir = join_paths(gnome_todo_datadir, meson.project_name())
gnome_todo_pkgincludedir = join_paths(gnome_todo_includedir, meson.project_name())
gnome_todo_pkglibdir = join_paths(gnome_todo_libdir, meson.project_name())

gnome_todo_pluginsdir = join_paths(gnome_todo_pkglibdir, 'plugins')
gnome_todo_schemadir = join_paths(gnome_todo_datadir, 'glib-2.0', 'schemas')

soversion = 0
current = 0
revision = 0
libversion = '@0@.@1@.@2@'.format(soversion, current, revision)

gnome_todo_debug = get_option('buildtype').contains('debug')

cc = meson.get_compiler('c')

config_h = configuration_data()

config_h.set_quoted('GETTEXT_PACKAGE', meson.project_name())

# debug options
config_h.set('GNOME_TODO_ENABLE_DEBUG', gnome_todo_debug)
config_h.set('NDEBUG', not gnome_todo_debug)

# package
set_defines = [
  ['PACKAGE', meson.project_name()],
  ['PACKAGE_BUGREPORT', 'http://bugzilla.gnome.org/enter_bug.cgi?product=' + meson.project_name()],
  ['PACKAGE_NAME', meson.project_name()],
  ['PACKAGE_STRING', '@0@ @1@'.format(meson.project_name(), gnome_todo_version)],
  ['PACKAGE_TARNAME', meson.project_name()],
  ['PACKAGE_URL', 'https://wiki.gnome.org/Apps/Todo'],
  ['PACKAGE_VERSION', gnome_todo_version],
  ['VERSION', gnome_todo_version],
  # i18n
  ['GETTEXT_PACKAGE', meson.project_name()]
]

foreach define: set_defines
  config_h.set_quoted(define[0], define[1])
endforeach

# headers
check_headers = [
  ['HAVE_DLFCN_H', 'dlfcn.h'],
  ['HAVE_INTTYPES_H', 'inttypes.h'],
  ['HAVE_LOCALE_H', 'locale.h'],
  ['HAVE_MEMORY_H', 'memory.h'],
  ['HAVE_STDINT_H', 'stdint.h'],
  ['HAVE_STDLIB_H', 'stdlib.h'],
  ['HAVE_STRINGS_H', 'strings.h'],
  ['HAVE_STRING_H', 'string.h'],
  ['HAVE_SYS_STAT_H', 'sys/stat.h'],
  ['HAVE_SYS_TYPES_H', 'sys/types.h'],
  ['HAVE_UNISTD_H', 'unistd.h']
]

foreach header: check_headers
  config_h.set(header[0], cc.has_header(header[1]))
endforeach

# functions
check_functions = [
  # i18n
  ['HAVE_DCGETTEXT', 'dcgettext'],
  ['HAVE_GETTEXT', 'gettext'],
  ['HAVE_ICONV', 'iconv']
]

if host_machine.system().contains('darwin')
  check_functions += [
    ['HAVE_CFLOCALECOPYCURRENT', 'CFLocaleCopyCurrent'],
    ['HAVE_CFPREFERENCESCOPYAPPVALUE', 'CFPreferencesCopyAppValue']
  ]
endif

foreach func: check_functions
  config_h.set(func[0], cc.has_function(func[1]))
endforeach

# compiler flags
common_flags = ['-DHAVE_CONFIG_H']
compiler_flags = []

if gnome_todo_debug
  test_cflags = [
    '-fno-strict-aliasing',
    '-Wcast-align',
    '-Wdeclaration-after-statement',
    '-Wformat=2',
    '-Winit-self',
    '-Winline',
    '-Wmissing-declarations',
    '-Wmissing-format-attribute',
    '-Wmissing-include-dirs',
    '-Wmissing-noreturn',
    '-Wmissing-prototypes',
    '-Wnested-externs',
    '-Wno-error=unused-parameter',
    '-Wno-error=missing-field-initializers',
    '-Wno-missing-field-initializers',
    '-Wno-unused-parameter',
    '-Wold-style-definition',
    '-Wpacked',
    '-Wredundant-decls',
    '-Wshadow',
    '-Wstrict-prototypes',
    '-Wswitch-enum',
    '-Wundef',
    '-Wunused-but-set-variable',
    '-Wwrite-strings',
  ]

  foreach cflag: test_cflags
    if cc.has_argument(cflag)
      compiler_flags += [cflag]
    endif
  endforeach
endif

add_project_arguments(common_flags + compiler_flags, language: 'c')

glib_dep = dependency('glib-2.0', version: '>= 2.43.4')
gtk_dep = dependency('gtk+-3.0', version: '>= 3.22.0')

gnome_todo_deps = [
  glib_dep,
  gtk_dep,
  dependency('gio-2.0', version: '>= 2.43.4'),
  dependency('gmodule-export-2.0'),
  dependency('goa-1.0', version: '>= 3.2.0'),
  dependency('libpeas-1.0', version: '>= 1.17'),
  cc.find_library('m', required: true)
]

configure_file(
  output: 'config.h',
  configuration: config_h
)

gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')

top_inc = include_directories('.')
src_inc = include_directories('src', 'src/engine', 'src/interfaces', 'src/notification')

data_dir = join_paths(meson.source_root(), 'data')
po_dir = join_paths(meson.source_root(), 'po')

subdir('plugins')
subdir('src')
subdir('data')
subdir('po')

enable_gtk_doc = get_option('enable-gtk-doc')
if enable_gtk_doc
  subdir('doc/reference')
endif

meson.add_install_script('meson_post_install.py')

output = '\n        gnome-todo ' + gnome_todo_version + '\n'
output += '        ==================\n\n'
output += '        prefix:        ' + gnome_todo_prefix + '\n'
output += '        compiler:      ' + cc.get_id() + '\n'
output += '        global flags:  ' + ' '.join(compiler_flags) + ' '.join(get_option('c_link_args')) + '\n'
output += '        release:       ' + (not gnome_todo_debug).to_string() + '\n'
output += '        documentation: ' + enable_gtk_doc.to_string() + '\n'
output += '    Plugins:\n\n'
output += '        Dark theme .............. ' + get_option('enable-dark-theme-plugin').to_string() + '\n'
output += '        Run in Background ....... ' + get_option('enable-background-plugin').to_string() + '\n'
output += '        Scheduled panel ......... ' + get_option('enable-scheduled-panel-plugin').to_string() + '\n'
output += '        Score ................... ' + get_option('enable-score-plugin').to_string() + '\n'
output += '        Today panel ............. ' + get_option('enable-today-panel-plugin').to_string() + '\n'
output += '        Unscheduled panel ....... ' + get_option('enable-unscheduled-panel-plugin').to_string() + '\n'
output += '        Todo.txt ................ ' + get_option('enable-todo-txt-plugin').to_string() + '\n'
output += '        Todoist ................. ' + get_option('enable-todoist-plugin').to_string() + '\n'
output += '        Now type \'ninja -C ' + meson.build_root() + '\' to build ' + meson.project_name()
message(output)
