// converts smartctl output for nvme disks into old format
// purpose: input for munin smart_ plugin
// (c) 2021, A. Raphael
// License:  free
// Warranty: none

#include <stdio.h>
#include <cstring>
#include <stdlib.h>


int   getline2 ( char* buf, int max, FILE* inf );
char* replacews( char* desc );
long  strtolx  ( const char* p );


int main (int argc, char* argv[])
{
    char log[1000];
    int l;
    int value;
    double dvalue;
    char *pT;

    FILE *cmd;
    // overwriting all arguments passed by plugin, except drive
    char smartarg[100] = "smartctl -A ";
    strcat( smartarg, argv[ argc-1 ] );

    int on = 0;
    int n = 0;
    printf( "ID# ATTRIBUTE_NAME FLAG VALUE WORST THRESH TYPE UPDATED WHEN_FAILED RAW_VALUE\n" );

    cmd = popen( smartarg, "r");
    while ( ! feof( cmd ) )
    {
	l = getline2( log, 999, cmd);

	// starting point of info-section
	if ( strncmp("SMART/Health Information", log, 24) == 0 ) { on = 1; continue; }
	// stop output at next blank line
	if ( on && l <= 0 )  break;

        if (on)
	{
	   // Description may contain whitescpaces, but always ends with a colon
	   pT = strstr( log, ":" );
	   if ( ! pT )  continue;

	   // munin smart-plugin splits lines at whitespaces
	   // Therefore the description-field must NOT comtain spaces! Replace them with underscores
	   char* desc = replacews(log);

	   // Fetch smart-value
	   dvalue = strtolx(pT+1);
	   value = (int)dvalue;

	   // Squeeze all values into range 0-100
	   char prefixlevel = 0;
	   long multiplier = 1;
	   int exp = 0;
	   while ( dvalue > 100 )  { dvalue /= 10;  multiplier *= 10;  exp += 1; }

	   // output in old format
	   if ( multiplier > 1 )
	   {
		if ( exp > 3 )
			printf( "%i %s_(/1e%i) 0x0 %.2f %i 0 - - - %i\n", n+1, desc, exp, dvalue, (int)dvalue, value );
		else
			printf( "%i %s_(/%i) 0x0 %.2f %i 0 - - - %i\n", n+1, desc, (int)multiplier, dvalue, (int)dvalue, value );
	   }
	   else
		printf( "%i %s 0x0 %.2f %i 0 - - - %i\n", n+1, desc, dvalue, (int)dvalue, value );

	   n++;
	}

    }

    pclose(cmd);
    return 0;
}



int getline2(char* buf, int max, FILE* inf)
{
        int l;

        *buf = 0;
        fgets( buf, max, inf );
        l = strlen(buf);
        if    ( l>0 && buf[l-1]=='\n' ) buf[--l] = 0;
        while ( l>0 && buf[l-1]==' '  ) buf[--l] = 0;

        return l;
}


// identifier must not comtain spaces! Replace them with underscores
char descbuf[256];
char* replacews( char* desc )
{
	strncpy( descbuf, desc, 255 );  descbuf[255] = 0;
	char* p = descbuf;

	while ( *p )
	{
		if ( *p == ':' )  { *p = 0;  break; }
		if ( *p == ' ' || *p == '.' )  { *p = '_'; }
		p++;
	}
	return descbuf;
}


// fetch smart value ( removing thousand-separators)
long strtolx( const char* p )
{
	char buf[128];
	char *dp = (char*)p;
	int  l=0;

	// strip leading whitespaces
	while (*dp==' ' || *dp=='\t') dp++;

	// is hex value
	if (dp[0]=='0' && dp[1]=='x') return strtol( dp, NULL, 16 );

	// strip thousand-separators
	while ( l<127 && *dp!=0 && strchr( "0123456789,", *dp )!=NULL )
	{
		if ( *dp != ',' )  buf[l++] = *dp;
		dp++;
	}
	buf[l] = 0;

	return atol(buf);
}
