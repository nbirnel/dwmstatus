#define BATT_NOW        "/sys/class/power_supply/BAT1/energy_now"
#define BATT_FULL       "/sys/class/power_supply/BAT1/energy_full"
#define BATT_STATUS       "/sys/class/power_supply/BAT1/status"
#define WIFI       "/sys/class/net/wlan0/operstate"

#include <errno.h>

#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>

char *tzpacific = "America/Los_Angeles";

static Display *dpy;

char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

void
settz(char *tzname)
{
	setenv("TZ", tzname, 1);
}

char *
getbattery(){
    long now, full = 0;
    char *status = malloc(sizeof(char)*12);
    char s = '?';
    FILE *fp = NULL;
    if ((fp = fopen(BATT_NOW, "r"))) {
        fscanf(fp, "%ld", &now);
        fclose(fp);
        fp = fopen(BATT_FULL, "r");
        fscanf(fp, "%ld", &full);
        fclose(fp);
        fp = fopen(BATT_STATUS, "r");
        fscanf(fp, "%s", status);
        fclose(fp);
        if (strcmp(status,"Charging") == 0)
            s = '+';
        if (strcmp(status,"Discharging") == 0)
            s = '-';
        if (strcmp(status,"Full") == 0)
            s = '=';
        return smprintf("%c%ld%%", s,(now/(full/100)));
    }
    else return smprintf("");
}

char *

getwifi(){
    /* FIXME I don't know what I'm doing with malloc */
    char *status = malloc(sizeof(char)*12);
    FILE *fp = NULL;
    if ((fp = fopen(WIFI, "r"))) {
        fscanf(fp, "%s", status);
        fclose(fp);
        return smprintf("%s", status);
    }
    else return smprintf("");
}

char *
mktimes(char *fmt, char *tzname)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;

	memset(buf, 0, sizeof(buf));
	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL) {
		perror("localtime");
		exit(1);
	}

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		exit(1);
	}

	return smprintf("%s", buf);
}

void
setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char *
loadavg(void)
{
	double avgs[3];

	if (getloadavg(avgs, 3) < 0) {
		perror("getloadavg");
		exit(1);
	}

	return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}

int
main(void)
{
	char *status;
    char *wifi;
    char *bat;
	char *avgs;
	char *tmlosangeles;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}

	for (;;sleep(1)) {
		avgs = loadavg();
        bat = getbattery();
        wifi = getwifi();
		tmlosangeles = mktimes("W %W D %j %H:%M:%S %Z %a %d-%b-%Y", tzpacific);

		status = smprintf("W: %s B: %s L:%s %s",
				wifi, bat, avgs, tmlosangeles);
		setstatus(status);
		free(wifi);
		free(bat);
		free(avgs);
		free(tmlosangeles);
		free(status);
	}

	XCloseDisplay(dpy);

	return 0;
}
