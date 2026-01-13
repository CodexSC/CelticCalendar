<p align="center">
  <img src="images/Celtic-knot.svg" width="120"/>
</p>

# 🕯️ Celtic Calendar 🕯️

<p align="center">
<b><i>“May the knots of time reveal the wisdom of the ancients.”</i></b>
</p>

<p align="center">
<img src="images/Coligny_calendar.png" width="400"/>
</p>

<p align="center">
<b>A C project for exploring, simulating, and visualizing the ancient Celtic calendar system, with astronomical precision and festival lore.</b>
</p>

---

## ✨ Features & Functionality

- **Celtic Calendar Simulation**
  - Visualizes months, days, and the unique structure of the Coligny calendar.
  - Highlights the dual coicise (half-months), intercalary months, and authentic day counts.
- **Astronomical Calculations**
  - Computes Julian Day, lunar phases, solar positions, and zodiac signs for any date.
- **Festival Detection**
  - Automatically marks major Celtic festivals (Samhain, Imbolc, Beltane, Lughnasadh) and astronomical cross-quarters.
- **TUI (Terminal User Interface)**
  - Navigate the calendar with arrow keys in a richly decorated, knotwork-inspired interface.
  - ASCII/Unicode art, color highlights, and Celtic knot motifs throughout.
- **Coligny Tablet Notation**
  - Displays reconstructed day notations, auspicious/inauspicious days, and festival markers as on the original bronze tablet.
- **Search & Navigation**
  - Jump to any Gregorian date, move forward/backward by lunar months, or return to today.
- **Test & Debug Utilities**
  - Standalone programs for validating astronomical and calendar logic.

---

## 🗂️ Project Structure

```
├── astronomy.c/h         # Astronomical calculations
├── calendar.c/h          # Calendar logic
├── data.c/h              # Data tables and constants
├── festivals.c/h         # Festival logic
├── glyphs.c/h            # Unicode/ASCII rendering, Coligny notation
├── main.c                # Main entry point
├── main_interactive.c    # TUI entry point
├── ui_ncurses.c/h        # Terminal UI (ncurses)
├── celtic_calendar_tui   # Main TUI executable
├── test_*.c              # Test and debug programs
└── .dist/                # (Optional) Build outputs
```

---

## 🖼️ Example Output

![Celtic Calendar Example](images/Coligny_calendar_months.png)

---

## ⚙️ Build & Run

```bash
# Build the TUI (recommended):
gcc -Wall -O2 main_interactive.c ui_ncurses.c glyphs.c data.c astronomy.c festivals.c calendar.c -lncursesw -lm -o celtic_calendar_tui

# Run the interactive Celtic Calendar:
./celtic_calendar_tui

# Or build and run the CLI version:
gcc -Wall -O2 main.c astronomy.c calendar.c data.c festivals.c glyphs.c -lm -o celtic_calendar
./celtic_calendar

# Test utilities:
gcc -o test_astro test_astro.c astronomy.c
gcc -o test_dates test_dates.c calendar.c data.c
./test_astro
./test_dates
```

---

## 📚 References

- [Coligny Calendar (Wikipedia)](https://en.wikipedia.org/wiki/Coligny_calendar)
- [Celtic Festivals](https://en.wikipedia.org/wiki/Celtic_calendar#Festivals)

---

## 📝 License

MIT License. See `LICENSE` file for details.

---

## 🙏 Acknowledgements

- Images: Wikimedia Commons
- Research: Academic and open-source Celtic calendar reconstructions

---

<p align="center">
  <img src="images/Celtic-knot.svg" width="120"/>
</p>
