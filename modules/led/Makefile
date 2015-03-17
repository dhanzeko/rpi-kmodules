SHELL:=/bin/bash

prog:
	-rm -rf build
	-mkdir build
	@cd build ;\
	cmake .. ;\
	make ;


run:
	@cat /proc/led
	@echo 


clean:
	-rm -rfv build/ 
	-rm -fv $(shell find . | grep ~$$)
	-cd kmod/; make clean


