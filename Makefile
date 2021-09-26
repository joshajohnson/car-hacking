VERSION?=0.1
NAME?=
DESIGNER?=Josh Johnson

new:
	git submodule update --init --recursive --progress
	cd josh-kicad-lib && git checkout master && git pull
	cd josh-kicad-lib && bash setup.sh "$(VERSION)" "$(NAME)" "$(DESIGNER)"

init:
	rm -rf .git
	git init
	git submodule add git@github.com:joshajohnson/josh-kicad-lib.git
