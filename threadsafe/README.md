# Threadsafe algoritms

## SPSC
> Tests will be posted here soon. Preliminary results: tested spsc_queue, looks blazing fast, especially on Apple ARM silicon... 

- spsc_queue 
> A fast bounded single-producer single-consumer queue. 

- unbounded_spsc_queue
> A pretty fast single-producer single-consumer unbounded queue using block-lists under the hood. 

## MPMC 
- mutex_queue 
> A simple mutex-based mpmc queue
