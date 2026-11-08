# Master Duel Switcher

Switch *Yu-Gi-Oh! Master Duel* accounts in one click without redownloading 10 GB of card assets on alt accounts.

Works on **Windows** and **Linux** (including Steam Deck / SteamOS).

---

## Installation

### Windows (PowerShell)

Open PowerShell and run:

```powershell
irm https://raw.githubusercontent.com/bebekgalaksi/MasterDuel-Switcher/main/install.ps1 | iex
```

### Linux / Steam Deck (Terminal)

Open a terminal or Konsole and run:

```bash
curl -sSL https://raw.githubusercontent.com/bebekgalaksi/MasterDuel-Switcher/main/install.sh | bash
```

---

## Manual Download

Prefer a standalone portable folder? Download pre-built binaries directly from [Releases](https://github.com/bebekgalaksi/MasterDuel-Switcher/releases/latest):

- **Windows:** Download and extract `MasterDuelSwitcher-Windows-x64.zip`, then run `MasterDuelSwitcher.exe`.
- **Linux:** Download `MasterDuelSwitcher-Linux-x86_64.tar.gz`, extract, and run `MasterDuelSwitcher`.

---

## What It Does

- **Bypasses the 10 GB Redownload:** Master Duel forces alt accounts to download ~10–12 GB of card artwork and audio separately into their own folder. The switcher automatically links alt accounts to your primary 10 GB data so they use 0 GB extra space and skip the download.
- **One-Click Account Switch:** Switches your active Steam profile and boots directly into Master Duel without having to log out manually in Steam.
- **Bypasses "Who's Playing?":** Disables the Steam account chooser screen that intercepts startup.
- **Game Running Guard:** Won't switch or touch files while `masterduel.exe` is running to prevent cache corruption.
- **Safe & Offline:** Zero memory injection, zero DLL hooks, and zero network calls. Works purely through local Steam configs and filesystem links.

---

## How to Add New Accounts

1. Open Steam, log into your new alt account once, and check **Remember Password**.
2. Open Master Duel Switcher—your new account appears in the list automatically.
3. Click **Switch & Play**. The app handles the 10 GB data link automatically in the background.

---

## Uninstallation

**Windows (PowerShell):**
```powershell
irm https://raw.githubusercontent.com/bebekgalaksi/MasterDuel-Switcher/main/uninstall.ps1 | iex
```

**Linux (Terminal):**
```bash
curl -sSL https://raw.githubusercontent.com/bebekgalaksi/MasterDuel-Switcher/main/uninstall.sh | bash
```

---

## License

[MIT](LICENSE)
