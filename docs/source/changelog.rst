=========
Changelog
=========

Versioning is generally based on `semantic versioning <https://semver.org>`_.

1.0.0 (2018-4-9)
----------------

User Notes
~~~~~~~~~~

Initial release for our `ISPASS'18 publication <http://ieeexplore.ieee.org/search/searchresult.jsp?newsearch=true&queryText=ispass>`_.

Events
******

*Control Flow* events are not currently implemented, although a provisional
interface is provided in :doc:`events`.

Frontends
*********

Valgrind is fairly well supported. It can be slow at event generation, although a faster version is in the works.

DynamoRIO is less well supported, but should generate basic memory, compute, and synchronization events fairly well. 
This is planned to be updated.

The Intel PT perf frontend was implemented as a proof-of-concept and there is a lot of room for increased
event support and optimization. 

Developer Notes
~~~~~~~~~~~~~~~
A new Valgrind implementation (**gengrind**) is close to being completed.
Currently we are working on how best to implement branching with VEX IR, which seems to only support limited branching.
The function tracking component of gengrind is based off of Callgrind.
A few bugs may be present in this new function tracking since we stripped away some of the Callgrind specific
functionality, such as cost-centers and cache simulations.

The DynamoRIO frontend requires some extra event checks to make sure the raw instructions it sees are properly binned.
Additionally, some restrictions in its internal lock implementations make detecting and generating synchronization
events more costly than we would expect. Specifically, we cannot directly generate events in function intercepts,
and must instead set a flag that gets checked in every basic block. Also, we plan to look into thread-private
code-caches to optimize ROI checks that happen at the beginning of each basic block.
