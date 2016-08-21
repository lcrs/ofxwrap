# Ofxwrap
This is an initial look at running an OFX plugin inside a Spark plugin inside Flame or Smoke or Flare or Flame Assist. For now it can just about load Neat Video, open its UI, save and load its setups and process frames.  More complicated plugins would require a bunch more work.  Lil' video link:

[![Ofxwarp demo video](http://img.youtube.com/vi/tMaSOM4HADc/3.jpg)](http://www.youtube.com/watch?v=tMaSOM4HADc)

## How
Tested with Smoke 2016.0.1 and Flame Assist 2016.2 and should work with other recent versions.

You'll need Neat Video 4 Pro OpenFX - tested with 4.0.0 on OSX 10.10.5 and 4.1.1 on RHEL 6.5.

The Neat binary should be in one of these places:
```
/Library/OFX/Plugins/NeatVideo4.ofx.bundle/Contents/MacOS/NeatVideo4.ofx
/usr/discreet/sparks/neat_unpacked/NeatVideo4.ofx.bundle/Contents/MacOS/NeatVideo4.ofx
/usr/OFX/Plugins/NeatVideo4.ofx.bundle/Contents/Linux-x86-64/NeatVideo4.ofx
/usr/discreet/sparks/neat_unpacked/NeatVideo4.ofx.bundle/Contents/Linux-x86-64/NeatVideo4.ofx
```

Then load the Spark as usual and go wild.  It will ask for the Neat licence the first time it's used.

## Caveats
Aside from crashes, the main problem is that multiple instances of the Spark really don't behave.  Having multiple nodes in Batch or simultaneously using a node in Batch and a timeline soft effect will likely not work.

When using the menus at the top of the Neat window, sometimes you have to press and release instead of clicking and dragging through the choices.

If you full-screen Neat and then close it, you're left with a black screen, but hit apple-tab and you'll be back.

Setting the environment variable OFXWRAP_DEBUG will cause a huge slew of messages in the shell log which may indicate what is going wrong.  For example:

```
env OFXWRAP_DEBUG=1 /usr/discreet/smoke_2016.0.1.case00584709/bin/startApplication
tail /usr/discreet/log/smoke201601case00584709_tinface_shell.log
```

## Innards
This goes slightly against the grain of the OFX API by merely catching the plugin's API calls and responding to each in a targeted manner, rather than creating the set of host-side objects and properties which is pretty clearly inferred by the structure of the API.

Here's the rough order of operations:
- Flame calls `SparkMemoryTempBuffers()`
  - we respond by calling `SparkMemRegisterBuffer()` 11 times to set up extra buffers for the temporal frames of input Neat requires
- Flame calls `SparkInitialise()`
  - we use `dlopen()` to load in the OFX binary, then `dlsym()` to get the addresses of the plugin's `OfxGetNumberOfPlugins()` and `OfxGetPlugin()` functions
  - we call `OfxGetPlugin(0)` to get the plugin struct containing the `mainEntry()` function, which we'll use to send actions to the plugin
  - we send the Load action to the plugin via its `mainEntry()` function
    - the plugin calls back our `fetchSuite()` function to get pointers to all the other API functions
    - the plugin calls a few of the property suite API functions to get our host name and version
  - we send the `Describe` action via `mainEntry()`
    - the plugin calls a few more property suite functions to set properties telling us its name, what bit depths it supports and various other things
  - we send the `DescribeInContext` action
    - the plugin sets a ton of properties telling us what inputs it wants and what parameters it uses
  - we send the `CreateInstance` action
    - the plugin tells us where its internal instance data lives, and asks us for the initial values of its parameters, which we supply from our global state variables
- Flame usually calls `SparkIOEvent()` to let us know it's loading the most recently used setup
  - we load the state of the OFX plugin's parameters from disk - we use a setup file next to the normal Spark setup file for this, with `_ofxsetup` on the end
  - we send the `DescribeInContext` and `CreateInstance` actions again to force the plugin to re-read the new state of its parameters
    - the plugin calls back a ton of OFX API functions to tell us about the new instance, as before
- Flame calls `SparkProcess()` whenever it needs to render a frame
  - we use `SparkGetFrame()` to get the image buffers for the 11 input frames that may be required
  - we allocate separate buffers that will be passed to the OFX plugin as inputs - sadly OFX uses RGBA buffers where Flame uses RGB, so we convert whatever bit-depth Flame is supplying us with to 32-bit float RGBA
  - we allocate a buffer ready for the OFX output frame
  - we send the `Render` action to the plugin
    - the plugin calls back a ton of OFX API functions to ask details about the image buffers
    - the plugin grabs the input images from our buffers, does work on them and fills our output buffer
  - we convert the OFX 32-bit float RGBA output buffer back to whatever bit-depth Flame is working at
  - we hand the finished result buffer off to Flame, which it displays or writes to disk
- at some point Flame will call our UI button callback functions
  - we send the `InstanceChanged` action along with property set handles indicated that the change was due to a button being clicked
    - the plugin calls back tons of OFX API functions asking about what changed and why
    - the plugin opens the Neat interface window where you can sample noise and play with all the usual settings
    - during this the plugin pulls images from the input buffers we set up inside `SparkProcess()`, so it can draw the picture in its own window
    - the plugin calls the OFX `parms_SetValue()` function to update its parameters with new stuff from the UI
      - we take those new values and update our own global state
  - the plugin returns from the `InstanceChanged` action and we can continue with more `SparkProcess()` calls for more frames
- at some point Flame will call `SparkIOEvent()` to let us know it's saving the setup - either because the user hit Save, or because it needs to copy the Spark node, or unload and reload the Spark whilst keeping the state the same
  - we write our global state to disk in a file ending `_ofxsetup` next to the main Spark setup file
- eventually Flame calls `SparkUninitialise()` when the Spark is no longer in use
  - we tidy up a ton of stuff and call `dlclose()` to unload the OFX binary

### Problems
Ideally we'd respect the intentions of the OFX API and only load the plugin once, let it describe itself once, then create multiple instances as needed.  Sadly this would be a huge amount of work because getting individual Spark nodes to talk to each other is tough - each one is loaded as a distinct shared library, so communication has to be via SHM, a file on disk or a socket.  Coordinating all the actions necessary to keep just one copy of the OFX plugin and let other nodes set and get its data structures does not really bear thinking about...

As a consequence of this we probably leak quite a bit of memory, because instances can't be destroyed in the conventional way - if we try to, the plugin calls back asking us awkward questions about previous instances, the state of which we don't have access to from the current Spark's globals :(

I explored using a similar method to Flame itself - copying the OFX plugin binary to a unique file for each instance of the Spark, running install_name_tool on each.  This caused horrid problems with the ObjC runtime, which detects multiple linking of the same class and soon crashes.  Search and replacing on the offending class names in the binaries only made it worse. Possibly this could work on Linux, which presumably doesn't use ObjC.
