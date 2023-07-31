<center>  
<h1>QuirkWM</h1>  
</center>  
QuirkWM is a simple yet highly effective tiling window manager (WM) designed to improve your workflow. Written in C and utilizing X11, QuirkWM offers a minimalistic interface focused on simplicity and usability. Its tiling arrangement maximizes screen real estate, offering a streamlined working environment.

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
git clone https://github.com/letrad/QuirkWM.git
```

2. Change into the QuirkWM directory:
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
