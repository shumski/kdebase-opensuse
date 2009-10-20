#! /bin/sh

set -e

desktop="`xdg-user-dir DESKTOP 2>/dev/null`"
if test -z "$desktop"; then
    desktop=$HOME/Desktop
fi

if test -e /usr/share/applications/YaST2/live-installer.desktop ; then
   if [ ! -e "$desktop/live-installer.desktop" -a -e "/usr/share/kde4/config/SuSE/default/live-installer.desktop" ]; then
        mkdir -p "$desktop"
        cp /usr/share/kde4/config/SuSE/default/live-installer.desktop "$desktop/"
        chmod u+x "$desktop/live-installer.desktop"
   fi
fi

mkdir -p $HOME/.kde4/share/config
cp -a /usr/share/kde4/config/SuSE/default/lowspacesuse.live $HOME/.kde4/share/config/lowspacesuse
cp -a /usr/share/kde4/config/SuSE/default/kdedrc.live $HOME/.kde4/share/config/kdedrc
cp -a /usr/share/kde4/config/SuSE/default/kcmnspluginrc.live $HOME/.kde4/share/config/kcmnspluginrc

mkdir -p $HOME/.config/autostart
cp -a /usr/share/kde4/config/SuSE/default/kupdateapplet-autostart.desktop.live $HOME/.config/autostart/kupdateapplet-autostart.desktop
cp -a /usr/share/kde4/config/SuSE/default/beagled-autostart.desktop.live $HOME/.config/autostart/beagled-autostart.desktop

# first generate ksycoca, it will be used by nsplugincan
/usr/bin/kbuildsycoca4
/usr/bin/nspluginscan
# this also has quite a big cost during the first startup
/usr/lib*/kde4/libexec/kconf_update
# create the final ksycoca
/usr/bin/kbuildsycoca4

# the cache is hostname specific, so don't hardcode "build24". at least "linux"
mv $HOME/.kde4/cache-* $HOME/.kde4/cache-linux