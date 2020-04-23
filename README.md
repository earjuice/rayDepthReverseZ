# rayDepthReverseZ
Cinder testing gl_FragDepth writing, shadertoy style for downstream rendering.

testing gl_FragDepth writing, shadertoy style for downstream rendering.. 
also supports reverse-z. 
There's another method which combines ray depth with a previously rendered fbo depth buffer.. in post.
This example demonstrates custom ray depth in Pre which is more efficient, less cumbersome but also only limited to solid objects.  
The post technique, however, can utilize transparencies, volumetrics but is a little heavier on the gpu and requires rendering into an fbo beforehand. 

this example uses cinder blocks:
https://github.com/rezaali/Cinder-AppPaths
https://github.com/rezaali/Cinder-LiveCode

you'll have to tweak apppaths and resources.h to get getAppSupportPath function to work properly or you could just remove the live-code feature..
Now that Cinder supports GLAD. I also did a pull request on the master branch for a reverseZ build of cinder from Paul's.
https://github.com/paulhoux/Cinder/tree/reverse-z
You'll need that for the reverse z define to work..
cheers
