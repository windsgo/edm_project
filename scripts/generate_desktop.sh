#!/bin/bash

# 获取脚本所在的绝对路径
SCRIPT_PATH=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

# 构建需要插入到 .desktop 文件中的路径
EXEC_PATH="${SCRIPT_PATH}/../run_withaudio.sh"
ICON_PATH="${SCRIPT_PATH}/App.png"
WORK_PATH="${SCRIPT_PATH}/../"

# 构造 .desktop 文件内容
desktop_entry="[Desktop Entry]
Name=数控程序
Exec=${EXEC_PATH}
Icon=${ICON_PATH}
Path=${WORK_PATH}
Type=Application
Categories=Application;
Version=1.0
Terminal=true
StartupNotify=true"

# 将内容写入 qsedm.desktop 文件
file_name=App.desktop
echo "$desktop_entry" > "${SCRIPT_PATH}/${file_name}"

rm -f ~/.local/share/applications/${file_name}

cp ${SCRIPT_PATH}/${file_name} ~/.local/share/applications/

if [ -d "${HOME}/Desktop" ]; then
desktop_link_name="${HOME}/Desktop/${file_name}"
rm -f ${desktop_link_name}
ln -s ~/.local/share/applications/${file_name} ${desktop_link_name}

chmod +x ${desktop_link_name}
fi

if [ -d "${HOME}/桌面" ]; then
desktop_link_name="${HOME}/桌面/${file_name}"
rm -f ${desktop_link_name}
ln -s ~/.local/share/applications/${file_name} ${desktop_link_name}

chmod +x ${desktop_link_name}
fi