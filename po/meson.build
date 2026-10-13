i18n = import('i18n')

add_project_arguments(f'-DGETTEXT_PACKAGE="@heimdall_package_name@"',
	language: ['c', 'cpp', 'objc'])
i18n.gettext(heimdall_package_name,
	args: [
		'--keyword=_',
		'--keyword=N_',
		'--keyword=_NS',
		'--keyword=qtr',
		'--keyword=Q_',
		'--language=C++',
		'--keyword=heimdall_ngettext:1,2',
		'--keyword=heimdall_pgettext:1c,2',
		'--keyword=_PNS:1c,2',
		'--add-comments=xgettext:',
		'--from-code=UTF-8',
	],
	install_dir: localedir_path)
