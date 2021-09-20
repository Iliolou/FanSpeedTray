qtPrepareTool(LCONVERT, lconvert)
isEmpty(LCONVERT_LANGS): LCONVERT_LANGS = el
LCONVERT_OUTPUTS =
for(lang, LCONVERT_LANGS) {
    lang_files =
    for(pat, LCONVERT_PATTERNS) {
        lang_files += $$files($$[QT_INSTALL_TRANSLATIONS]/$${pat}_$${lang}.qm)
    }
    outfile = $$OUT_PWD/qt_$${lang}.qm
    !isEmpty(lang_files): system("$$LCONVERT -i $$join(lang_files, ' ') -o $$outfile"): LCONVERT_OUTPUTS += $$outfile
}
qm_res.files = $$LCONVERT_OUTPUTS
qm_res.base = $$OUT_PWD
qm_res.prefix = "/"