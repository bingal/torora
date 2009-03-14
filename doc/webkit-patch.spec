
Prevent Disclosing Visited Sites Through CSS
--------------------------------------------
POC: http://ha.ckers.org/weird/CSS-history.cgi
Setting: QWebSettings::PreventUserProfiling
Files altered: WebCore/css/CSSStyleSelector.cpp

Strategy: WebKit returns PseudoVisited if a link in a CSS stylesheet has been visited.
          If PreventUserProfiling is enabled, WebKit will return PseudoLink instead.

Result: QWebSettings::PreventUserProfiling defeats the attack.



Prevent Disclosing Screen and Window Size/Resolution Information
----------------------------------------------------------------
POC: Theoretical, http://browserspy.dk/screen.php, http://browserspy.dk/window.php
Setting: QWebSettings::PreventUserProfiling
Files altered: WebCore/page/DOMWindow.cpp
               WebCore/page/Screen.cpp

Strategy: Same approach as Torbutton. Window size and Screen size are rounded by 50
          pixels to reduce the potential uniqueness of the user's configuration.
          The various javascript properties always return as follows:

          WebCore/page/DOMWindow.cpp:
          outerHeight() = outerHeight() rounded to the nearest 50 px
          outerWidth()  = outerWidth() rounded to the nearest 50 px
          innerHeight() = innerHeight() rounded to the nearest 50 px
          innerWidth()  = innerWidth() rounded to the nearest 50 px
          screenX()     = screenX() rounded to the nearest 50 px
          screenY()     = screenY() rounded to the nearest 50 px
          scrollX()     = scrollX() rounded to the nearest 50 px
          scrollY()     = scrollY() rounded to the nearest 50 px

          WebCore/page/Screen.cpp
          height()      = height()  rounded to the nearest 50 px
          width()       = width()  rounded to the nearest 50 px
          colorDepth()  = 24
          pixelDepth()  = 24
          availLeft()   = availLeft() rounded to the nearest 50 px
          availTop()    = availTop() rounded to the nearest 50 px
          availHeight() = availHeight() rounded to the nearest 50 px
          availWidth()  = availWidth() rounded to the nearest 50 px

Result: QWebSettings::PreventUserProfiling defeats the attack.


Prevent Disclosing Timezone Information
---------------------------------------
POC: Theoretical, http://browserspy.dk/screen.php, http://browserspy.dk/window.php
Setting: QWebSettings::PreventUserProfiling
Files altered: JavaScriptCore/runtime/DatePrototype.cpp
               JavaScriptCore/runtime/DateConstructor.cpp
               JavaScriptCore/runtime/DateMath.cpp

Strategy: Same approach as Torbutton. Once PreventUserProfiling is enabled the default
          Date object is created as UTC. Attempts to set the Date object using localtime
          (e.g. new Date('10:10:10')) also use UTC rather than actual localtime. Calls
          to getTimeZoneOffset() returns 0 explicitly.

          The following Date javascript methods always return UTC time:

          getTime
          getFullYear
          getYear
          getMonth
          getDate
          getDay
          getHours
          getMinutes
          getSeconds
          getMilliseconds
          getUTCFullYear
          getUTCMonth
          getUTCDate
          getUTCDay
          getUTCHours
          getUTCMinutes
          getUTCSeconds
          getUTCMilliseconds
          toUTCString
          toGMTString
          toString
          toLocaleString
          toLocaleTimeString
          toLocaleDateString
          toDateString
          toTimeString

          The following Date javascript methods always return 0:

          getTimezoneOffset

          The following Date javascript methods always set UTC time:

          setTime
          setFullYear
          setYear
          setMonth
          setDate
          setDay
          setHours
          setMinutes
          setSeconds
          setMilliseconds
          setUTCFullYear
          setUTCMonth
          setUTCDate
          setUTCDay
          setUTCHours
          setUTCMinutes
          setUTCSeconds
          setUTCMilliseconds

Result: QWebSettings::PreventUserProfiling defeats the attack.


Prevent Disclosing Locale/Platform Information
----------------------------------------------


pref("extensions.torbutton.spoof_charset",'iso-8859-1,*,utf-8');
pref("extensions.torbutton.spoof_language",'en-us, en');
pref("extensions.torbutton.spoof_locale",'en-US');

pref("extensions.torbutton.appname_override","Netscape");
pref("extensions.torbutton.appversion_override","5.0 (Windows; LANG)");
pref("extensions.torbutton.platform_override","Win32");
pref("extensions.torbutton.oscpu_override", "Windows NT 5.1");
pref("extensions.torbutton.useragent_override", "Mozilla/5.0 (Windows; U; Windows NT 5.1; LANG; rv:1.9.0.6) Gecko/2009011913 Firefox/3.0.6");

pref("extensions.torbutton.productsub_override","2009011913");
pref("extensions.torbutton.buildID_override","0");
pref("extensions.torbutton.useragent_vendor", "");
pref("extensions.torbutton.useragent_vendorSub","");
