#ifndef KFMPARSER_H
#define KFMPARSER_H


#pragma once

#include <QString>
#include <QStringList>

namespace KFM {

struct ParseResult {
    QString compiledText;   // same as targTxt.Text in VB (lines of hex bytes)
    bool wroteFile = false; // true if outputFile was written
};

    /**
    * Parse a KFM program text and optionally write a "toe" style file with 0x.., 0x.., 0x.., entries.
    *
    * @param inputText    The full source text (equivalent to txt.Text in VB).
    * @param outputFile   If non-empty and writeToeFile==true, the file to write (VB opened as #1).
    * @param writeToeFile Whether to write the 0x.., 0x.., 0x.., output file (frmDirList.chkCompileToeFile).
    * @return ParseResult containing compiledText and wroteFile flag.
    */
    ParseResult parseKFMProgram(const QString &inputText, const QString &outputFile = QString(), bool writeToeFile = false);
} // namespace KFM







#endif // KFMPARSER_H
