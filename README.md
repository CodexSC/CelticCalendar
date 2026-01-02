# Celtic Calendar

> **Image Setup:**
>
> To display images in this README on GitHub, please download the following files and place them in a new `images/` folder in this directory:
>
> - [Coligny_calendar.png](https://upload.wikimedia.org/wikipedia/commons/6/6e/Coligny_calendar.png)
> - [Calendrier_de_Coligny_03.jpg](https://upload.wikimedia.org/wikipedia/commons/2/2a/Calendrier_de_Coligny_03.jpg)
> - [Coligny_calendar_months.png](https://upload.wikimedia.org/wikipedia/commons/2/2e/Coligny_calendar_months.png)
> - [Celtic-knot.svg](https://upload.wikimedia.org/wikipedia/commons/2/2b/Celtic-knot.svg)
>
> After downloading, your folder should look like:
>
> ```
> Celtic Calendar/
> ├── images/
> │   ├── Coligny_calendar.png
> │   ├── Calendrier_de_Coligny_03.jpg
> │   ├── Coligny_calendar_months.png
> │   └── Celtic-knot.svg
> ```
>
> The images will then display correctly on GitHub and in Markdown viewers.


<!--
  To display images reliably on GitHub, download the following images and place them in the `images/` directory:
  - Coligny_calendar.png: https://upload.wikimedia.org/wikipedia/commons/6/6e/Coligny_calendar.png
  - Calendrier_de_Coligny_03.jpg: https://upload.wikimedia.org/wikipedia/commons/2/2a/Calendrier_de_Coligny_03.jpg
  - Coligny_calendar_months.png: https://upload.wikimedia.org/wikipedia/commons/2/2e/Coligny_calendar_months.png
  - Celtic-knot.svg: https://upload.wikimedia.org/wikipedia/commons/2/2b/Celtic-knot.svg
-->

![Celtic Calendar Banner](images/Coligny_calendar.png)

A C project for calculating, simulating, and exploring the ancient Celtic calendar system, including astronomical and festival computations.

---

## The Original Coligny Calendar Artifact


<p align="center">
  <img src="images/Calendrier_de_Coligny_03.jpg" alt="Coligny Calendar Artifact" width="500"/>
</p>

<p align="center"><em>The bronze fragments of the Coligny Calendar, discovered in France (Wikimedia Commons)</em></p>

---

## Features

- **Astronomical Calculations:**
  - Julian Day, lunar phases, solar positions, and more.
- **Celtic Calendar Simulation:**
  - Month and festival calculations based on reconstructed Celtic timekeeping.
- **Festival Detection:**
  - Automatic identification of major Celtic festivals (Samhain, Imbolc, Beltane, Lughnasadh).
- **Debug/Test Utilities:**
  - Multiple test programs for validating calendar logic and astronomical calculations.

---

## Project Structure

```
├── astronomy.c/h         # Astronomical calculations
├── calendar.c/h          # Calendar logic
├── data.c/h              # Data tables and constants
├── festivals.c/h         # Festival logic
├── glyphs.c/h            # Glyph rendering (if any)
├── main.c                # Main entry point
├── test_*.c              # Test and debug programs
├── celtic_calendar       # Main executable
├── celtic_chronometre    # Chronometer tool
└── beta Version/         # Experimental code
```

---

## Example Output

![Celtic Calendar Example](images/Coligny_calendar_months.png)

---

## Build & Run

```bash
# Compile all sources
gcc -o celtic_calendar main.c astronomy.c calendar.c data.c festivals.c glyphs.c

# Run the main program
./celtic_calendar
```

Or use the provided test programs:

```bash
gcc -o test_astro test_astro.c astronomy.c
gcc -o test_dates test_dates.c calendar.c data.c
./test_astro
./test_dates
```

---

## References

- [Coligny Calendar (Wikipedia)](https://en.wikipedia.org/wiki/Coligny_calendar)
- [Celtic Festivals](https://en.wikipedia.org/wiki/Celtic_calendar#Festivals)

---

## License

MIT License. See `LICENSE` file for details.

---

## Acknowledgements

- Images: Wikimedia Commons
- Research: Academic and open-source Celtic calendar reconstructions

---

![Celtic Knot](images/Celtic-knot.svg)
