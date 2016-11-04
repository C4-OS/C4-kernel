Various tasks that need to be done, in no particular order

## TODO
- [ ] memory management
    - [ ] implement flexpages
    - [ ] add support for smaller/larger slabs in the kernel slab allocator,
          rather than the fixed-size bitmap

- [ ] IPC
    - [ ] add more fields to message\_t, like type, sender, etc
    - [ ] allow recievers to specify which thread(s) they want to recieve from
    - [ ] add map, unmap, and grant messages
    - [ ] maybe add a 'buffer' field in messages which lets any message have
          a map operation optionally associated with it

- [ ] general
    - [ ] implement loading the root task
    - [ ] implement capabilities
    - [ ] implement a framebuffer driver and basic windowing system
	- [ ] add more stuff to the todo list
