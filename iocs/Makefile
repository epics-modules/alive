TOP = ..
include $(TOP)/configure/CONFIG
DIRS := $(wildcard *IOC)
include $(TOP)/configure/RULES_TOP

uninstall: uninstall_iocs
uninstall_iocs:
	$(MAKE) -C aliveExIOC uninstall
.PHONY: uninstall uninstall_iocs

realuninstall: realuninstall_iocs
realuninstall_iocs:
	$(MAKE) -C aliveExIOC realuninstall
.PHONY: realuninstall realuninstall_iocs
