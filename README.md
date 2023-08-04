<!--

<h1 align="center">QuirkWM</h1>  
QuirkWM stands tall as a remarkable tiling window manager (WM), combining simplicity and high effectiveness to revolutionize your productivity. Crafted in C and harnessing the power of X11 (the xlib library), QuirkWM presents a seamless, user-centric interface that sets a new standard for usability. Embracing a minimalist approach, it empowers you to effortlessly navigate and optimize your workspace, maximizing every inch of your screen for an unrivaled working environment. Say hello to efficiency, meet QuirkWM!


## 🚀 Features

- **Tiling Window Management**: Organizes your windows into a tiling pattern, eliminating any wasted space and allowing for easy navigation.
- **Keyboard Shortcuts 🎹**: QuirkWM supports intuitive keyboard shortcuts to create, close, and navigate between windows.
- **Lightweight Design 🪶**: As a minimalistic window manager, QuirkWM is fast and efficient, perfect for those who want a responsive system without unnecessary extras.
- **Gap Control 🖼️**: Adjust the gaps between windows for a more visually appealing setup.
- **Focus Control 🔍**: Easily navigate between the windows using keyboard shortcuts to focus on different parts of your work.

## 📦 Installation
### Dependencies
Make sure you have the X11 development libraries installed on your system.
```bash
sudo apt-get install libx11-dev # Debian
sudo pacman -S libx11 # Arch Linux
... 
```
On other systems, use the corresponding package manager and package name.

### Building from Source
1. **Clone the QuirkWM repository**:
```
git clone --recursive https://github.com/letrad/QuirkWM.git
```
 *QuirkWM has dependancies so you'll need to use `--recursive` to grab all those goodies along with it.*

2. **Change into the QuirkWM directory**:
```
cd QuirkWM
```
3. **Create a build directory and navigate into it**:
```bash
mkdir build
cd build
```
4. **Configure & Compile**:
 ```bash
 cmake ..
 make
```
5. **Add QuirkWM to .xinitrc or use startx**:
```bash
startx ./QuirkWM
```
## 🎮 Usage
### Key Bindings

- **Super + T**: Open a new terminal window (kitty).
- **Super + Q**: Quit the window manager.
- **Super + Left Arrow**: Focus the previous window.
- **Super + Right Arrow**: Focus the next window.

## 🤝 Contributing
Contributions to QuirkWM are welcome! Please read the contributing guidelines and submit pull requests or open an issue.

## 📜 License
QuirkWM is open source and available under the Mozilla Public License 2.0. See the LICENSE file for details.

-->
# QuirkWM
> A simple and easy to use tiling windowmanager

QuirkWM aims to provide a lightweight and efficient window management experience for users who prefer a keyboard-centric environment. With its focus on simplicity and modularity, QuirkWM targets power users, developers, and those looking to streamline their workflow by minimizing mouse usage, while allowing a certain level of customization through a configuration file. Its design principles emphasize performance, ease of use, and the ability to extend functionality through integration with other command-line tools and applications.

# ⚠️ EXPERIMENTAL
Please use this at your own risk, it has not been thorougly tested and can cause issues to your sytstem.

<!-- ![](header.png) -->

## Installation

Right now there aren't any releases use the Development setup.

## Usage example

<!-- Add screen shots-->

The Config has to be located at: `~/.config/quirk/config.toml`

### Basic Config example
```toml
[wm]
gap=10

[preferences]
terminal="st"
```

_For more examples and guides, please refer to the [Wiki][wiki]._

## Development setup

### Unix/Linux

1. Using make
```sh
git clone --recursive https://github.com/letrad/QuirkWM.git
cd QuirkWM
make all
```

2. Using cmake
```sh
git clone --recursive https://github.com/letrad/QuirkWM.git
cd QuirkWM
mkdir build
cd build
cmake ..
make
```

Run using startx
```sh
startx ./quirkwm
```

Start using Xephyr (Only on X11)
```sh
Xephyr :9 -screen 1280x720 &
DISPLAY=:9 ./quirkwm
```

## Release History

* 0.0.1
    * Work in progress

## Meta

letradical – [@letradical](https://discord.com) <!-- – YourEmail@example.com -->

Distributed under the Mozilla Public License 2.0 license. See ``LICENSE`` for more information.

[https://github.com/letrad/QuirkWM](https://github.com/letrad/QuirkWM)

## Contributing

1. Fork it (<https://github.com/letrad/QuirkWM/fork>)
2. Create your feature branch (`git checkout -b feature/fooBar`)
3. Commit your changes (`git commit -am 'Add some fooBar'`)
4. Push to the branch (`git push origin feature/fooBar`)
5. Create a new Pull Request

<!-- Markdown link & img dfn's -->
[wiki]: https://github.com/letrad/QuirkWM/wiki
