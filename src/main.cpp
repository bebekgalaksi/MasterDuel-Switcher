#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QThread>
#include <QDateTime>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <vector>

#ifdef Q_OS_WIN
#include <QSettings>
#include <windows.h>
#endif

const QString APP_ID = "1449850";

struct SteamUser {
    QString steamId;
    QString accountName;
    QString personaName;
    bool autoLogin = false;
    QString avatarPath;
};

// Find Steam installation root
QString getSteamPath() {
#ifdef Q_OS_WIN
    // Check 64-bit and 32-bit registry keys for Steam install path
    QSettings reg64("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Valve\\Steam", QSettings::NativeFormat);
    QString path = reg64.value("InstallPath").toString();
    if (!path.isEmpty() && QDir(path).exists()) return path;

    QSettings reg32("HKEY_LOCAL_MACHINE\\SOFTWARE\\Valve\\Steam", QSettings::NativeFormat);
    path = reg32.value("InstallPath").toString();
    if (!path.isEmpty() && QDir(path).exists()) return path;

    QString defaultPath = "C:/Program Files (x86)/Steam";
    if (QDir(defaultPath).exists()) return defaultPath;
    return defaultPath;
#else
    QString nativePath = QDir::homePath() + "/.local/share/Steam";
    if (QDir(nativePath).exists()) return nativePath;

    QString flatpakPath = QDir::homePath() + "/.var/app/com.valvesoftware.Steam/.local/share/Steam";
    if (QDir(flatpakPath).exists()) return flatpakPath;

    return nativePath;
#endif
}

// Find registry path (Linux only; Windows uses native registry)
#ifndef Q_OS_WIN
QString getRegistryPath(const QString &steamPath) {
    QString p1 = QDir::homePath() + "/.steam/registry.vdf";
    if (QFile::exists(p1)) return p1;

    QString p2 = steamPath + "/registry.vdf";
    if (QFile::exists(p2)) return p2;

    QString p3 = QDir::homePath() + "/.var/app/com.valvesoftware.Steam/.steam/registry.vdf";
    if (QFile::exists(p3)) return p3;

    return p1;
}
#endif

// Find Master Duel LocalData directory
QString findMasterDuelLocalData(const QString &steamPath) {
    // 1. Direct check in default common directory
    QString defaultCommon = steamPath + "/steamapps/common";
    QDir commonDir(defaultCommon);
    if (commonDir.exists()) {
        for (const QString &d : commonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (d.contains("Master Duel", Qt::CaseInsensitive)) {
                QString cand = commonDir.filePath(d) + "/LocalData";
                if (QDir(cand).exists()) return cand;
            }
        }
    }

    // 2. Check libraryfolders.vdf
    QFile libFile(steamPath + "/config/libraryfolders.vdf");
    if (libFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString text = QString::fromUtf8(libFile.readAll());
        libFile.close();
        QRegularExpression rx(R"foo("path"\s+"([^"]+)")foo");
        auto it = rx.globalMatch(text);
        while (it.hasNext()) {
            auto m = it.next();
            QString rawPath = m.captured(1);
#ifdef Q_OS_WIN
            rawPath.replace("\\\\", "/");
#endif
            QString libPath = rawPath + "/steamapps/common";
            QDir dir(libPath);
            if (dir.exists()) {
                for (const QString &d : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                    if (d.contains("Master Duel", Qt::CaseInsensitive)) {
                        QString cand = dir.filePath(d) + "/LocalData";
                        if (QDir(cand).exists()) return cand;
                    }
                }
            }
        }
    }

    return "";
}

// Check if Master Duel is currently running
bool isMasterDuelRunning() {
#ifdef Q_OS_WIN
    QProcess proc;
    proc.start("tasklist", {"/FI", "IMAGENAME eq masterduel.exe", "/NH"});
    proc.waitForFinished();
    return QString::fromUtf8(proc.readAllStandardOutput()).contains("masterduel.exe", Qt::CaseInsensitive);
#else
    return (QProcess::execute("pgrep", {"-f", "masterduel.exe"}) == 0);
#endif
}

// Check if Steam is actively running
bool isSteamRunning() {
#ifdef Q_OS_WIN
    QProcess proc;
    proc.start("tasklist", {"/FI", "IMAGENAME eq steam.exe", "/NH"});
    proc.waitForFinished();
    return QString::fromUtf8(proc.readAllStandardOutput()).contains("steam.exe", Qt::CaseInsensitive);
#else
    // 1. Check steam.pid file
    QString pidFile = QDir::homePath() + "/.steam/steam.pid";
    if (QFile::exists(pidFile)) {
        QFile f(pidFile);
        if (f.open(QIODevice::ReadOnly)) {
            QString pidStr = QString::fromUtf8(f.readAll()).trimmed();
            f.close();
            bool ok = false;
            qint64 pid = pidStr.toLongLong(&ok);
            if (ok && pid > 0) {
                if (QDir(QString("/proc/%1").arg(pid)).exists()) return true;
            }
        }
    }
    // 2. Fallback check using pgrep
    if (QProcess::execute("pgrep", {"-f", "steamwebhelper"}) == 0) return true;
    if (QProcess::execute("pgrep", {"-f", "ubuntu12_32/steam"}) == 0) return true;
    if (QProcess::execute("pgrep", {"-x", "steam"}) == 0) return true;
    return false;
#endif
}

// Cleanly stop Steam
void stopSteam() {
    if (!isSteamRunning()) return;

#ifdef Q_OS_WIN
    QProcess::execute("cmd.exe", {"/c", "start", "steam://exit"});

    for (int i = 0; i < 25; ++i) {
        QThread::msleep(200);
        if (!isSteamRunning()) return;
    }

    QProcess::execute("taskkill", {"/F", "/IM", "steam.exe"});
    for (int i = 0; i < 15; ++i) {
        QThread::msleep(200);
        if (!isSteamRunning()) return;
    }
#else
    // Graceful request first
    QProcess::execute("steam", {"-shutdown"});

    // Wait up to 6 seconds for clean shutdown
    for (int i = 0; i < 30; ++i) {
        QThread::msleep(200);
        if (!isSteamRunning()) return;
    }

    // Force SIGTERM if hanging
    QProcess::execute("pkill", {"-TERM", "-f", "ubuntu12_32/steam"});
    QProcess::execute("pkill", {"-TERM", "-f", "steamwebhelper"});
    for (int i = 0; i < 15; ++i) {
        QThread::msleep(200);
        if (!isSteamRunning()) return;
    }
#endif
}

// Parse loginusers.vdf
std::vector<SteamUser> loadSteamUsers(const QString &loginusersPath, const QString &avatarDir) {
    std::vector<SteamUser> users;
    QFile file(loginusersPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return users;

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    QRegularExpression userBlockRx(R"foo("(\d{17})"\s*\{([^}]+)\})foo");
    auto it = userBlockRx.globalMatch(content);
    while (it.hasNext()) {
        auto m = it.next();
        SteamUser user;
        user.steamId = m.captured(1);
        QString body = m.captured(2);

        QRegularExpression accRx(R"foo("AccountName"\s+"([^"]+)")foo");
        auto accMatch = accRx.match(body);
        if (accMatch.hasMatch()) user.accountName = accMatch.captured(1);

        QRegularExpression personaRx(R"foo("PersonaName"\s+"([^"]+)")foo");
        auto personaMatch = personaRx.match(body);
        if (personaMatch.hasMatch()) user.personaName = personaMatch.captured(1);

        QRegularExpression autoRx(R"foo("AutoLogin"\s+"([^"]+)")foo");
        auto autoMatch = autoRx.match(body);
        if (autoMatch.hasMatch()) user.autoLogin = (autoMatch.captured(1) == "1");

        QString avatar = avatarDir + "/" + user.steamId + ".png";
        if (QFile::exists(avatar)) user.avatarPath = avatar;

        users.push_back(user);
    }
    return users;
}

// Update AutoLogin in loginusers.vdf
bool setAutoLoginUser(const QString &loginusersPath, const QString &targetSteamId) {
    QFile file(loginusersPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    QString currentId = "";
    QStringList lines = content.split('\n');
    QStringList outLines;

    QRegularExpression idRx(R"foo(^\s*"(\d{17})")foo");
    QRegularExpression autoLoginRx(R"foo(^(\s*"AutoLogin"\s+)"[^"]*"(.*)$)foo");
    QRegularExpression timestampRx(R"foo(^(\s*"timestamp"\s+)"[^"]*"(.*)$)foo");

    qint64 now = QDateTime::currentSecsSinceEpoch();

    for (const QString &line : lines) {
        auto idMatch = idRx.match(line);
        if (idMatch.hasMatch()) {
            currentId = idMatch.captured(1);
        }

        auto autoMatch = autoLoginRx.match(line);
        if (autoMatch.hasMatch()) {
            QString val = (currentId == targetSteamId) ? "1" : "0";
            outLines.append(autoMatch.captured(1) + "\"" + val + "\"" + autoMatch.captured(2));
            continue;
        }

        auto timeMatch = timestampRx.match(line);
        if (timeMatch.hasMatch() && currentId == targetSteamId) {
            outLines.append(timeMatch.captured(1) + "\"" + QString::number(now) + "\"" + timeMatch.captured(2));
            continue;
        }

        outLines.append(line);
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return false;
    file.write(outLines.join('\n').toUtf8());
    file.close();
    return true;
}

// Update AutoLoginUser in Registry (Windows Native Registry or Linux registry.vdf)
bool setAutoLoginRegistry(const QString &registryPath, const QString &accountName) {
#ifdef Q_OS_WIN
    Q_UNUSED(registryPath);
    QSettings reg("HKEY_CURRENT_USER\\Software\\Valve\\Steam", QSettings::NativeFormat);
    reg.setValue("AutoLoginUser", accountName);
    reg.setValue("RememberPassword", 1);
    reg.sync();
    return true;
#else
    QFile file(registryPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    QStringList lines = content.split('\n');
    QStringList outLines;
    QRegularExpression autoLoginRx(R"foo(^(\s*"AutoLoginUser"\s+)"[^"]*"(.*)$)foo");

    bool found = false;
    for (const QString &line : lines) {
        auto m = autoLoginRx.match(line);
        if (m.hasMatch()) {
            outLines.append(m.captured(1) + "\"" + accountName + "\"" + m.captured(2));
            found = true;
        } else {
            outLines.append(line);
        }
    }

    if (!found) {
        QStringList insertLines;
        bool inSteam = false;
        for (const QString &l : outLines) {
            insertLines.append(l);
            if (l.contains("\"Steam\"") && !l.contains("SteamPID")) {
                inSteam = true;
            } else if (inSteam && l.contains("{")) {
                insertLines.append(QString("\t\t\t\t\t\"AutoLoginUser\"\t\t\"%1\"").arg(accountName));
                inSteam = false;
            }
        }
        outLines = insertLines;
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return false;
    file.write(outLines.join('\n').toUtf8());
    file.close();
    return true;
#endif
}

// Disable "AlwaysShowUserChooser" (the "Who's playing?" startup screen) in config.vdf
bool disableAlwaysShowUserChooser(const QString &configPath) {
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    QStringList lines = content.split('\n');
    QStringList outLines;
    QRegularExpression chooserRx(R"foo(^(\s*"AlwaysShowUserChooser"\s+)"[^"]*"(.*)$)foo");

    bool found = false;
    for (const QString &line : lines) {
        auto m = chooserRx.match(line);
        if (m.hasMatch()) {
            outLines.append(m.captured(1) + "\"0\"" + m.captured(2));
            found = true;
        } else {
            outLines.append(line);
        }
    }

    if (!found) {
        QStringList insertLines;
        bool inAuth = false;
        for (const QString &l : outLines) {
            insertLines.append(l);
            if (l.contains("\"Auth\"")) {
                inAuth = true;
            } else if (inAuth && l.contains("{")) {
                insertLines.append("\t\t\t\"AlwaysShowUserChooser\"\t\t\"0\"");
                inAuth = false;
            }
        }
        outLines = insertLines;
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return false;
    file.write(outLines.join('\n').toUtf8());
    file.close();
    return true;
}

// Automatically ensure alt accounts are symlinked to primary 10GB folder
bool autoEnsureSymlinks(const QString &localDataPath, QString &primaryName, int &linkedCount) {
    linkedCount = 0;
    primaryName = "";
    if (localDataPath.isEmpty()) return false;

    QDir localDir(localDataPath);
    // Find primary folder: directory, not a symlink, contains '0000'
    for (const auto &entry : localDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (!entry.isSymLink() && !entry.fileName().startsWith(".")) {
            if (QDir(entry.absoluteFilePath() + "/0000").exists()) {
                primaryName = entry.fileName();
                break;
            }
        }
    }

    if (primaryName.isEmpty()) {
        for (const auto &entry : localDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (!entry.isSymLink() && !entry.fileName().startsWith(".")) {
                primaryName = entry.fileName();
                break;
            }
        }
    }

    if (primaryName.isEmpty()) return false;

    // Check all other account folders in LocalData
    for (const auto &entry : localDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (entry.fileName().startsWith(".")) continue;
        if (entry.fileName() == primaryName) continue;

        if (entry.isSymLink()) {
            linkedCount++;
        } else {
            // Replace standalone directory with symlink/junction to primary data
            QDir(entry.absoluteFilePath()).removeRecursively();
#ifdef Q_OS_WIN
            QString altNative = QDir::toNativeSeparators(entry.absoluteFilePath());
            QString primNative = QDir::toNativeSeparators(localDir.filePath(primaryName));
            QProcess::execute("cmd.exe", {"/c", "mklink", "/J", altNative, primNative});
#else
            QFile::link(primaryName, entry.absoluteFilePath());
#endif
            linkedCount++;
        }
    }
    return true;
}

// Create a rounded avatar icon
QIcon createRoundedAvatar(const QString &path, int size) {
    QPixmap src(path);
    if (src.isNull()) return QIcon();
    src = src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    QPixmap dest(size, size);
    dest.fill(Qt::transparent);

    QPainter painter(&dest);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath p;
    p.addRoundedRect(0, 0, size, size, 8, 8);
    painter.setClipPath(p);
    painter.drawPixmap(0, 0, src);
    return QIcon(dest);
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Apply dark modern styling
    app.setStyleSheet(R"(
        QWidget {
            background-color: #161922;
            color: #dbe2ef;
            font-family: 'Segoe UI', 'Ubuntu', 'Sans-Serif';
            font-size: 13px;
        }
        QListWidget {
            background-color: #1e2330;
            border: 1px solid #2d3548;
            border-radius: 8px;
            padding: 6px;
            outline: none;
        }
        QListWidget::item {
            background-color: #262c3d;
            border: 1px solid #323b52;
            border-radius: 6px;
            padding: 8px 12px;
            margin-bottom: 6px;
        }
        QListWidget::item:selected {
            background-color: #2b3954;
            border: 1px solid #00b4d8;
            color: #ffffff;
        }
        QListWidget::item:hover:!selected {
            background-color: #2d3447;
            border: 1px solid #3d4763;
        }
        QPushButton#launchBtn {
            background-color: #0077b6;
            color: #ffffff;
            font-weight: bold;
            font-size: 14px;
            border: none;
            border-radius: 6px;
            padding: 10px 20px;
        }
        QPushButton#launchBtn:hover {
            background-color: #0096c7;
        }
        QPushButton#launchBtn:pressed {
            background-color: #023e8a;
        }
        QPushButton#launchBtn:disabled {
            background-color: #2c3347;
            color: #64748b;
        }
    )");

    QString steamPath = getSteamPath();
#ifdef Q_OS_WIN
    QString registryPath = "";
#else
    QString registryPath = getRegistryPath(steamPath);
#endif
    QString configPath = steamPath + "/config/config.vdf";
    QString loginusersPath = steamPath + "/config/loginusers.vdf";
    QString avatarDir = steamPath + "/config/avatarcache";
    QString localDataPath = findMasterDuelLocalData(steamPath);

    // Automatic symlink enforcement
    QString primaryFolder;
    int linkedCount = 0;
    bool symlinkOk = autoEnsureSymlinks(localDataPath, primaryFolder, linkedCount);

    QWidget win;
    win.setWindowTitle("Master Duel Account Switcher");
    win.setFixedSize(500, 380);

    auto *mainLayout = new QVBoxLayout(&win);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    mainLayout->setSpacing(12);

    // Header title
    auto *titleLabel = new QLabel("<h2>Master Duel Account Switcher</h2>", &win);
    mainLayout->addWidget(titleLabel);

    auto *subtitleLabel = new QLabel("Select an account to launch directly into Master Duel:", &win);
    subtitleLabel->setStyleSheet("color: #94a3b8; font-size: 12px;");
    mainLayout->addWidget(subtitleLabel);

    // Account List
    auto *userList = new QListWidget(&win);
    userList->setIconSize(QSize(40, 40));
    mainLayout->addWidget(userList);

    std::vector<SteamUser> users = loadSteamUsers(loginusersPath, avatarDir);

    int activeIdx = 0;
    for (int i = 0; i < (int)users.size(); ++i) {
        const auto &u = users[i];
        QString displayText = QString("%1 (%2)").arg(u.personaName, u.accountName);
        if (u.autoLogin) {
            displayText += "  —  [Active in Steam]";
            activeIdx = i;
        }

        auto *item = new QListWidgetItem(displayText, userList);
        if (!u.avatarPath.isEmpty()) {
            item->setIcon(createRoundedAvatar(u.avatarPath, 40));
        }
        userList->addItem(item);
    }

    if (!users.empty()) {
        userList->setCurrentRow(activeIdx);
    } else {
        auto *warnItem = new QListWidgetItem("No saved Steam accounts found in loginusers.vdf", userList);
        userList->addItem(warnItem);
    }

    // Status area
    auto *statusLabel = new QLabel(&win);
    statusLabel->setWordWrap(true);
    if (!localDataPath.isEmpty() && symlinkOk) {
        statusLabel->setText(QString("<span style='color: #4ade80;'>✓ 10GB asset data shared automatically via symlink (Source: %1)</span>")
            .arg(primaryFolder));
    } else if (localDataPath.isEmpty()) {
        statusLabel->setText("<span style='color: #f87171;'>⚠ Master Duel LocalData folder not detected yet.</span>");
    } else {
        statusLabel->setText("<span style='color: #38bdf8;'>ℹ 1 account data profile detected.</span>");
    }
    statusLabel->setStyleSheet("font-size: 11px; padding: 2px;");
    mainLayout->addWidget(statusLabel);

    // Action button
    auto *launchBtn = new QPushButton("Switch & Play Master Duel", &win);
    launchBtn->setObjectName("launchBtn");
    launchBtn->setCursor(Qt::PointingHandCursor);
    if (users.empty()) launchBtn->setEnabled(false);
    mainLayout->addWidget(launchBtn);

    // Update button text on selection change
    auto updateButtonText = [&]() {
        int row = userList->currentRow();
        if (row >= 0 && row < (int)users.size()) {
            const auto &u = users[row];
            if (u.autoLogin) {
                launchBtn->setText(QString("Play Master Duel as %1").arg(u.personaName));
            } else {
                launchBtn->setText(QString("Switch to %1 & Play").arg(u.personaName));
            }
        }
    };
    QObject::connect(userList, &QListWidget::currentRowChanged, [&](int) {
        updateButtonText();
    });
    updateButtonText();

    // Launch action
    auto executeLaunch = [&]() {
        int row = userList->currentRow();
        if (row < 0 || row >= (int)users.size()) return;

        // 1. Check if Master Duel is currently running
        if (isMasterDuelRunning()) {
            QMessageBox::warning(&win, "Game Already Running",
                "Master Duel is currently running.\n\nPlease close Master Duel before switching accounts.");
            return;
        }

        const auto &targetUser = users[row];

        // 2. Ensure symlinks are up to date before launching
        QString pName;
        int lCount = 0;
        autoEnsureSymlinks(localDataPath, pName, lCount);

        // 3. Stop Steam if running to ensure a clean session handover
        launchBtn->setEnabled(false);
        if (isSteamRunning()) {
            launchBtn->setText("Closing Steam cleanly...");
            qApp->processEvents();
            stopSteam();
        }

        // 4. Update all Steam config files:
        // - loginusers.vdf (AutoLogin & timestamp)
        // - Registry (AutoLoginUser)
        // - config.vdf (AlwaysShowUserChooser = 0 to bypass "Who's playing?" prompt)
        launchBtn->setText("Setting active profile...");
        qApp->processEvents();
        setAutoLoginUser(loginusersPath, targetUser.steamId);
        setAutoLoginRegistry(registryPath, targetUser.accountName);
        disableAlwaysShowUserChooser(configPath);

        // Brief delay to ensure filesystem flush
        QThread::msleep(300);

        // 5. Launch Steam directly into Master Duel
        launchBtn->setText("Launching Steam & Master Duel...");
        qApp->processEvents();
#ifdef Q_OS_WIN
        QProcess::startDetached("cmd.exe", {"/c", "start", "steam://rungameid/" + APP_ID});
#else
        QProcess::startDetached("steam", {"steam://rungameid/" + APP_ID});
#endif

        // Close launcher
        QThread::msleep(600);
        QApplication::quit();
    };

    QObject::connect(launchBtn, &QPushButton::clicked, executeLaunch);
    QObject::connect(userList, &QListWidget::itemDoubleClicked, [&](QListWidgetItem *) {
        executeLaunch();
    });

    win.show();
    return app.exec();
}
