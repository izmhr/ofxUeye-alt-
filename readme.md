# Introduction

Capture frames from a uEye camera device

# License

MIT license (http://opensource.org/licenses/MIT)

# Other ofxUeye

Paulo Barcelos created a nifty ofxUeye at  https://github.com/paulobarcelos/ofxUeye/, I suggest checking there first.

# To build

* Clone this project into your `$(OF_ROOT)\addons`.
* Install IDS SDK into default directory `C:\Program Files\IDS`.
  * All `.h` files and `.lib` files you need must be correctly placed into the directories above.

# Does not support

* Continous capture (this requires you to work with windows events which make my skin crawl)
* Triggering (although I really wwant this, again I'd need to work with MFC windows events)
* Binning
* AOI

# Future development

ofxUeye-alt will be rewritten to implement ofxMachineVision in the future.

# Note from author

I'd have to say that IDS did a superbly awful job of making an API. They really need:

1. To get rid of functions that perform multiple tasks (e.g. get and set in one function, where return arguments can mean either error codes on set or result of get?)
2. Better documentation, cleaner samples (lots of console samples please!)
