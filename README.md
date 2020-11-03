# LightDM-Module-Greeter

A minimal but highly configurable single-user GTK3 greeter for LightDM.
Fork of [lightdm-mini-greeter](https://github.com/prikhi/lightdm-mini-greeter)

### Features
* Change rendering system to handle alpha channel.
* Add background-image display option
* Add module system
* Config backward compatible with mini-greeter 

### Module features
* Text from script output
* Refresh every given amount of time 
* Exec script on click

### Simple module

```
[module/name]
# Style was loaded from default `greeter-theme`
text = "Lorem ipsum"
```
For more read default .conf file.

## Install

You will need `automake`, `pkg-config`, `gtk+`, & `liblightdm-gobject` to build
the project.

Grab the source, build the greeter, & install it manually:

```sh
./autogen.sh
./configure --datadir /usr/share --bindir /usr/bin --sysconfdir /etc
make
sudo make install
```

Run `sudo make uninstall` to remove the greeter.

## Configure

Once installed, you should specify `lightdm-module-greeter` as your
`greeter-session` in `/etc/lightdm/lightdm.conf`. If you have multiple Desktop
Environments or Window Managers installed, you can specify the one to start by
changing the `user-session` option as well(look in `/usr/share/xsession` for
possible values).

Modify `/etc/lightdm/lightdm-module-greeter.conf` to customize the greeter. At
the very least, you will need to set the `user`.

You can test it out using LightDM's `test-mode`:

    lightdm --test-mode -d

### Keyboard layout

If your keyboard layout is loaded from your shell configuration files (`.bashrc`
for example) then it might not be possible to type certain characters after
installing lightdm-mini-greeter. You should consider modifying your 
[Xorg keyboard configuration](https://wiki.archlinux.org/index.php/Xorg/Keyboard_configuration#Using_X_configuration_files).

For example for a french keyboard layout (azerty) you should edit/create 
`/etc/X11/xorg.conf.d/00-keyboard.conf` with at least the following options:

```
Section "InputClass"
        Identifier "system-keyboard"
        MatchIsKeyboard "on"
        Option "XkbModel" "pc104"
        Option "XkbLayout" "fr"
EndSection
```

## License

GPL-3