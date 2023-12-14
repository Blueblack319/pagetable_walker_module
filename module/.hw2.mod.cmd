cmd_/home/hpcp/hw2_2018147593/module/hw2.mod := printf '%s\n'   hw2.o | awk '!x[$$0]++ { print("/home/hpcp/hw2_2018147593/module/"$$0) }' > /home/hpcp/hw2_2018147593/module/hw2.mod
