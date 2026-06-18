scons:=scons -j4

all:
	@$(scons)

clean:
	@$(scons) -c

copy:
	@$(scons) --copy -s

menuconfig:
	@$(scons) --menuconfig

rebuild:
	@$(scons) -c
	@$(scons)
