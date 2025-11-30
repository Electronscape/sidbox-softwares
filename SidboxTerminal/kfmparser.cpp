#include "kfmparser.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QVector>
#include <QDebug>

namespace KFM {

// internal structs
struct KFMInstruction {
    QString label;
    QString text;
};

struct LabelInfo {
    QString name;
    int lineIndex;
};

// op tables
static QVector<QString> s_opcodes;
static QVector<QString> s_varcodes;
static bool s_tablesInitialized = false;

static void InitOpTables()
{
    if (s_tablesInitialized) return;
    s_tablesInitialized = true;

    // opcode list transcribed from VB code (space separated)
    const QString ops = QStringLiteral(
        "NOP RES "
        "GTE GTO WAV FRQ FRA VOL PWM "
        "ADD SUB DIV MUL MOD RND "
        "ATT DCY SUS REL "
        "CMP JNE JIE JIG JIL JGE JLE JSR RET JMP "
        "STV MOV SHL SHR SWP TGL "
        "EFF ARP "
        "--- --- --- --- --- --- --- --- "
        "--- --- --- --- --- --- --- --- --- --- "
        "--- --- --- --- --- --- "
        "INF STP END"
        );
    s_opcodes = ops.split(' ', Qt::SkipEmptyParts);

    // variable names
    s_varcodes = QStringLiteral("XV0 XV1 XV2 XV3 XV4 XV5 XV6 XV7 XV8 XV9").split(' ', Qt::SkipEmptyParts);
}

static int GetOpcodeIndex(const QString &mnemonic)
{
    for (int i = 0; i < s_opcodes.size(); ++i) {
        if (s_opcodes[i].compare(mnemonic, Qt::CaseInsensitive) == 0) return i;
    }
    return -1;
}

static int GetVarCode(const QString &v)
{
    for (int i = 0; i < s_varcodes.size(); ++i) {
        if (s_varcodes[i].compare(v, Qt::CaseInsensitive) == 0) return 0x80 + i;
    }
    // fallback: if numeric string, return numeric value
    bool ok = false;
    int val = v.toInt(&ok, 10);
    if (ok) return val;
    return 0; // fallback
}

// tokenization: replace commas with spaces, split on whitespace, drop empties
static QStringList GetTokens(const QString &line)
{
    QString t = line;
    t.replace(',', ' ');
    // split on whitespace using regex to be robust
    QStringList parts = t.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    return parts;
}

// ParseParam: returns integer parameter value for p, using labels for label-reference resolution
static int ParseParam(const QString &param, int currentLine, const QVector<LabelInfo> &labels)
{
    QString p = param.trimmed();
    if (p.isEmpty()) return 0;

    // Hex literal 0xNN
    if (p.startsWith(QLatin1String("0x"), Qt::CaseInsensitive)) {
        bool ok = false;
        int v = p.mid(2).toInt(&ok, 16);
        return ok ? v : 0;
    }
    // variable like XV0
    if (p.startsWith(QLatin1Char('X'), Qt::CaseInsensitive)) {
        return GetVarCode(p);
    }
    // numeric or negative
    bool ok = false;
    int ival = p.toInt(&ok, 10);
    if (ok) return ival;

    // otherwise assume label: find label in labels and return relative offset (tgt - currentLine)
    int tgt = -1;
    for (int j = 0; j < labels.size(); ++j) {
        if (labels[j].name.compare(p, Qt::CaseInsensitive) == 0) {
            tgt = labels[j].lineIndex;
            break;
        }
    }
    if (tgt >= 0) return tgt - currentLine;
    return 0;
}

ParseResult parseKFMProgram(const QString &inputText, const QString &outputFile, bool writeToeFile)
{
    InitOpTables();
    ParseResult result;
    result.compiledText.clear();
    result.wroteFile = false;

    // Split input into lines (handle CRLF or LF)
    QStringList rawLines = inputText.split(QRegularExpression("\\r?\\n"), Qt::SkipEmptyParts);

    // Step 1: Preprocess lines, merge label-only with next
    QVector<KFMInstruction> parsed;
    parsed.reserve(rawLines.size());

    int i = 0;
    while (i < rawLines.size()) {
        QString line = rawLines[i].trimmed();
        if (line.isEmpty()) { ++i; continue; }

        if (line.endsWith(QLatin1Char(':'))) {
            QString labelPart = line.left(line.length() - 1).trimmed();
            QString codePart;
            if (i + 1 < rawLines.size()) {
                codePart = rawLines[i + 1].trimmed();
                ++i; // consume next as code
            } else {
                codePart = QStringLiteral("NOP");
            }
            KFMInstruction inst;
            inst.label = labelPart;
            inst.text = codePart;
            parsed.append(inst);
            ++i;
        } else {
            int colonPos = line.indexOf(QLatin1Char(':'));
            if (colonPos >= 0) {
                QString labelPart = line.left(colonPos).trimmed();
                QString codePart = line.mid(colonPos + 1).trimmed();
                KFMInstruction inst;
                inst.label = labelPart;
                inst.text = codePart;
                parsed.append(inst);
                ++i;
            } else {
                KFMInstruction inst;
                inst.label.clear();
                inst.text = line;
                parsed.append(inst);
                ++i;
            }
        }
    }

    // Step 2: Collect labels
    QVector<LabelInfo> labels;
    for (int idx = 0; idx < parsed.size(); ++idx) {
        if (!parsed[idx].label.isEmpty()) {
            LabelInfo li;
            li.name = parsed[idx].label;
            li.lineIndex = idx;
            labels.append(li);
        }
    }

    // Step 3: Resolve tokens and build output
    QStringList outLines; // each line: op p1 p2 as two-digit hex bytes concatenated like VB targTxt
    QFile outFile;
    QTextStream outFileStream;
    if (writeToeFile && !outputFile.isEmpty()) {
        outFile.setFileName(outputFile);
        if (outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            outFileStream.setDevice(&outFile);
            result.wroteFile = true;
        } else {
            // if cannot open, we still continue and just return compiled text
            qWarning() << "KFM: could not open output file:" << outputFile;
        }
    }

    for (int idx = 0; idx < parsed.size(); ++idx) {
        const QString &lineText = parsed[idx].text;
        QStringList tokens = GetTokens(lineText);

        int op = 0;
        int p1 = 0;
        int p2 = 0;

        if (!tokens.isEmpty()) {
            op = GetOpcodeIndex(tokens[0]);
            if (op < 0) op = 0;
        }
        if (tokens.size() >= 2) p1 = ParseParam(tokens[1], idx, labels);
        if (tokens.size() >= 3) p2 = ParseParam(tokens[2], idx, labels);

        // Ensure p1/p2 bytes are masked to 0..255 (as VB used p1 And &HFF)
        int p1b = p1 & 0xFF;
        int p2b = p2 & 0xFF;
        int opb  = op & 0xFF;

        // Format similar to VB: targTxt line is two hex bytes concatenated thrice then newline (they did right$("0" & Hex$(..), 2) ... )
        auto hex2 = [](int v)->QString {
            QString h = QString::number(v & 0xFF, 16).toUpper();
            if (h.length() == 1) h.prepend(QLatin1Char('0'));
            return h;
        };

        QString targLine = hex2(opb) + hex2(p1b) + hex2(p2b);
        outLines.append(targLine);

        // If writing toe file, VB wrote "0xAA, 0xBB, 0xCC, "
        if (result.wroteFile) {
            outFileStream << "0x" << hex2(opb) << ", 0x" << hex2(p1b) << ", 0x" << hex2(p2b) << ", " << "\n";
        }
    }

    if (result.wroteFile) {
        outFile.close();
    }

    // Build compiledText like targTxt.Text (lines joined with CRLF)
    result.compiledText = outLines.join("\r\n");
    return result;
}

} // namespace KFM
