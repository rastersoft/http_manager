# HTTP MANAGER #

This is a little manager that allows web applications to gain access to an underlying OS in an easy way. With it, a web app
can launch processes, read their output, and list files in the file system.

## USAGE ##

The idea behing http_manager is that web apps can use *xmlhttprequest()* to do several actions. Specifically they can launch a background process, read its stdout and/or stderr outputs (both at its end, or while they are running), or list the files in a folder.

## AVAILABLE COMMANDS ##

* **POST /run_program** launches a program, with its name (and optional path), parameters and so on in the data block. This
  program is launched inside a *shell*, and will not block the http manager because it is launched as a child, so it is possible
  to add *shell things* like redirections, pipes and so on.
  
  This command will return a JSON structure with a single element, named *pid*, with the pid number of the process launched.

* **POST /run_program_with_pipes** works like the previous command, but will redirect both *stdout* and *stderr*, to make them
  available by using the *get_result* and *get_partial_result* commands.

* **GET /get_result?pid=*pid* ** allows to get the status and (optionally) the output of a launched program. The *pid* parameters
  must contain the value returned by *run_program* or *run_program_with_pipes*. It returns a JSON structure with four elements:
  
  * *running* if it is *true*, the program is still running; if it is *false*, the program ended
  * *retval* only valid when *running* is *false*. It contains the return value given by the program
  * *stdout* if the program ended, it will contain all the output made to the standard output by the program; if not, it will be empty
  * *stderr* if the program ended, it will contain all the output made to the error output by the program; if not, it will be empty

* **GET /get_partial_result?pid=*pid* ** it works like *get_result*, but *stdout* and *stderr* will contain all the data generated
  since the last *get_partial_result* executed (or since the begining, if none was executed). This allows to show the output of the
  program in realtime, instead of having to wait for it to end.

* **POST /get_files** returns the files available in the path sent in the data block. It returns them as a JSON array, with one
  one element per file. Also, each element is a dictionary with three elements: *filename*, with the name of the file, *date*, with
  the last modification date of the file, and *isdir*, which is a boolean that specifies if this file is a folder or a regular file.

## CONTACTING THE AUTHOR ##

Sergio Costas Rodriguez  
http://www.rastersoft.com  
raster@rastersoft.com  
rastersoft@gmail.com  

