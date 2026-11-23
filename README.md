# Rod Ball Selector

A Geometry Dash mod that lets you choose the appearance used by Pulse Rods.

## Features

- Choose between **Normal**, **Rod 1**, **Rod 2**, and **Rod 3**.
- Save a Rod selection for each level.
- Open the selector from the level information screen or official level page.
- See the selected Rod represented on the selector button.

## How to use

1. Open a level's information page.
2. Press the Rod Ball Selector button near the top-right area of the page.
3. Choose **Normal**, **Rod 1**, **Rod 2**, or **Rod 3**.
4. Start the level. The selected appearance is used for that level.

**Normal** keeps the game's normal Rod appearance. When no choice has been saved for a level, the selection defaults to Normal. The selector button uses Rod 3 as its preview when no explicit selection has been saved.

## Requirements

- Geometry Dash
- Geode (see the target version in [`mod.json`](mod.json))
- [Node IDs](https://github.com/geode-sdk/NodeIDs)

The supported Geometry Dash version and required dependency versions are declared in `mod.json`.

## Installation

Once the mod is available on the Geode Index, install it through Geode's in-game mod browser.

During development, build the project with the Geode CLI:

```sh
geode build
```

## Building from source

1. Install the Geode SDK and Geode CLI for your platform.
2. Install the required SDK components and dependencies.
3. Clone or download this repository.
4. Run the build command from the project root:

   ```sh
   geode build
   ```

The generated `.geode` package can then be tested with your Geode installation.

## Issues and suggestions

Please report bugs or suggest improvements through the [GitHub Issues](https://github.com/budou-oisii/RodBallSelector/issues) page. When reporting a bug, include your Geometry Dash version, Geode version, operating system, steps to reproduce the issue, and a log or screenshot when useful.

## License

This project is distributed under the [YRC License 1.0](LICENSE.md).

The license applies only to materials created by Yukkuri RythmCraft and explicitly covered by it. Geometry Dash content, third-party libraries, Geode, Node IDs, and other third-party materials remain under their respective rights and licenses.

## Credits

Developed by **Yukkuri RythmCraft**.
