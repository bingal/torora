NOTE: All settings below, except those 'already implemented' are currently implemented
      using QWebSetting:PreventUserProfiling. The names will be finalized nearer
      patch submission.

Prevent Disclosing Visited Sites Through CSS
--------------------------------------------
POC: http://ha.ckers.org/weird/CSS-history.cgi
Setting: QWebSettings::ResistCSSVisitedExploit
Files altered: WebCore/css/CSSStyleSelector.cpp

Strategy: WebKit returns PseudoVisited if a link in a CSS stylesheet has been visited.
          If PreventUserProfiling is enabled, WebKit will return PseudoLink instead.

Result: QWebSettings::ResistCSSVisitedExploit defeats the attack.



Prevent Disclosing Screen and Window Size/Resolution Information
----------------------------------------------------------------
POC: Theoretical, http://browserspy.dk/screen.php, http://browserspy.dk/window.php
Setting: QWebSettings::MaskDOMWindowInformation
         QWebSettings::MaskScreenInformation
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
          height()      = window innerHeight()
          width()       = width()  rounded to the nearest 50 px
          colorDepth()  = 24
          pixelDepth()  = 24
          availLeft()   = 0
          availTop()    = 0
          availHeight() = window innerHeight()
          availWidth()  = window innerWidth()

Notes:  The client browser should resize it's window to 50px as well.

Result: QWebSettings::MaskDOMWindowInformation and  QWebSettings::MaskScreenInformation
        defeat the attack.


Prevent Disclosing Timezone Information
---------------------------------------
POC: Theoretical, http://browserspy.dk/screen.php, http://browserspy.dk/window.php
Setting: QWebSettings::MaskTimeZone
Files altered: JavaScriptCore/runtime/DatePrototype.cpp
               JavaScriptCore/runtime/DateConstructor.cpp
               JavaScriptCore/runtime/DateMath.cpp

Strategy: Same approach as Torbutton. Once PreventUserProfiling is enabled the default
          Date object is created as UTC. Attempts to set the Date object using localtime
          (e.g. new Date('10:10:10')) also use UTC rather than actual localtime. Calls
          to getTimeZoneOffset() returns 0 explicitly.

          The following Date javascript methods always return UTC time:

          getTime          getHours                  getUTCSeconds
          getFullYear      getMinutes                getUTCMilliseconds
          getYear          getSeconds                toUTCString
          getMonth         getMilliseconds           toGMTString
          getDate          getUTCFullYear            toString
          getDay           getUTCMonth               toLocaleString
                           getUTCDate                toLocaleTimeString
                           getUTCDay                 toLocaleDateString
                           getUTCHours               toDateString
                           getUTCMinutes             toTimeString

          The following Date javascript methods always return 0:

          getTimezoneOffset

          The following Date javascript methods always set UTC time:

          setTime                setMilliseconds
          setFullYear            setUTCFullYear
          setYear                setUTCMonth
          setMonth               setUTCDate
          setDate                setUTCDay
          setDay                 setUTCHours
          setHours               setUTCMinutes
          setMinutes             setUTCSeconds
          setSeconds             setUTCMilliseconds


Result: QWebSettings::PreventUserProfiling defeats the attack.


Prevent Disclosing Platform Information
----------------------------------------------

POC: Theoretical, http://browserspy.dk/screen.php, http://browserspy.dk/window.php
Setting: QWebSettings::MaskPlatform
Files altered: WebCore/page/Navigator.cpp
               WebCore/page/NavigatorBase.cpp
               JavaScriptCore/runtime/DateMath.cpp

Strategy: Same approach as Torbutton.


          Navigator Object
            appname = Netscape
            appversion = 5.0 (Windows; LANG)
            platform = Win32
            oscpu = Windows NT 5.1
            useragent = "Mozilla/5.0 (Windows; U; Windows NT 5.1; LANG; rv:1.9.0.6) Gecko/2009011913 Firefox/3.0.6"
            productsub = 2009011913
            buildID = 0
            useragent_vendor = ""
            useragent_vendorSub = ""

Prevent Disclosing Locale Information
----------------------------------------------
POC: Theoretical, http://browserspy.dk/screen.php, http://browserspy.dk/window.php
Setting: QWebSettings::MaskPlatform
Files altered: WebCore/page/Navigator.cpp
               WebCore/page/NavigatorBase.cpp
               JavaScriptCore/runtime/DateMath.cpp

Strategy: Same approach as Torbutton.


          Language Object

            charset = 'iso-8859-1,*,utf-8'
            language = 'en-us, en'
            locale = 'en-US'



Prevent Javascript access to the clipboard.
-------------------------------------------
  - Implemented by QWebSettings::JavascriptCanAccessClipboard


Same origin policy for file:// urls.
------------------------------------
    "Firefox 2's implementation of same origin policy allows file urls to read
    and submit arbitrary files from the local filesystem to arbitrary websites."
    (Torbutton Design Doc). Need to ensure that webkit does not have this problem.
    Disabling Javascript submit() can help mitigate against this attack. We could
    also add a preference to disable access to the network by javascript running
    with a local context.

          - Implemented by QWebSettings::AllowUniversalAccessFromFileURLs ?



Resist HTML Form attacks
------------------------
POC: Theoretical, http://www.remote.org/jochen/sec/hfpa/hfpa.pdf
Setting: QWebSettings::DisableJavascriptSubmit
Files altered: WebCore/bindings/js/JSHTMLFormElementCustom.cpp

Strategy: Disable the Javascript submit() method.


Password and Form Saving
------------------------
POC: "Since form fields can be read at any time by Javascript, this setting is a
      lot more important than it seems." (Torbutton Design Doc) Javascript timers
      could be used to read form fields in intermediate states, including states
      that contain values recorded from other sites' HTML forms.

Setting: QWebSettings::DisableJavascriptSubmit
Files altered: WebCore/html/HTMLFormElement.cpp

Strategy: Do not save or record values typed into HTML forms.

ToDo Items:
-----------
       - Forms with 'action' elements containing 'localhost' or '127.0.0.1' should be
         disabled. An alternative is to ban access to ports 8123, 8118, 9050 and 9051,
         or to ban access to localhost/127.0.0.1 completely.
         Ref: http://www.remote.org/jochen/sec/hfpa/hfpa.pdf
       - Need to prevent random external applications from launching without at least
         warning the user. 
