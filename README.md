# Entertaining Spice Pretending (Assignment 4 - CSF)

This is a two-player terminal card game inspired by *Spicy*, with simplified rules.

## 🎯 Objective

Players take turns playing cards face-down and declaring their value and spice. The opponent can either:
- **Believe the card**, and continue
- **Challenge the card** on its value or spice

Points are awarded for successful challenges. The game ends when the draw pile is empty.

## 🃏 How It Works

- Cards have a value (1–10) and spice (chili `c`, pepper `p`, or wasabi `w`)
- Players play or draw cards each turn
- Challenges can be made on spice or value
- Input is case-insensitive, space-tolerant, and validated
- Final scores are written back into the config file
