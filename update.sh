#!/bin/sh
# Update Riech-O-Mat from GitHub repository
dialog --title "Update Riech-O-Mat" --msgbox "This script will update Riech-O-Mat to the latest version. It will not alter your punchcard file! Press ESC to cancel and ENTER to continue," 10 50
# Return status of non-zero indicates cancel
if [ "$?" != "0" ]
then
  dialog --title "Update Riech-O-Mat" --msgbox "Update was \ canceled at your
  request." 10 50
else
  dialog --title "Update Riech-O-Mat" --infobox "Update in \ process..." 10 50  
  git pull git://github.com/sommeru/riech-o-mat.git master >|/tmp/update.riech-o-mat.log$$ 2>&1
  # zero status indicates update was successful
  if [ "$?" = "0" ]
    then
    dialog --title "Update Riech-O-Mat" --msgbox "Update completed sucessfully. Press ENTER to see log file." 10 50
    # Mark script with current date and time
    touch ~/.backup
  else
    # Backup failed, display error log
    dialog --title "Update Riech-O-Mat" --msgbox "Update failed. Press ENTER to see log file." 10 50
  fi
fi
dialog --title "Error Log" --textbox /tmp/update.riech-o-mat.log$$ 22 72
rm -f /tmp/update.riech-o-mat.lo
clear

