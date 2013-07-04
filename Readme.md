The Santa Claus Problem
====================================
Solution of simplified Santa Claus synchronization problem.

[video about this problem](http://www.youtube.com/watch?v=pqO6tKN2lc4)

#### Description

Santa and elfs are processes. Elfs building toys (simulation via sleep for rand time). Santa helps to elfs. Elfs needs help after each toy. Elf can go to vacation after building a specific number of toys. Santa can help to 3 elfs (or less if less than 3 working). Also there is main process.


#### How to compile?

	make
	
#### How to use?

	./santa a b c d
	
* **a**,**b**,**c**,**d** is integer numbers
* **a** is number of toys, which needed elf for vacation
* **b** is number of elfs
* **c** is maximum time for build one toy ***rand(0,c)***
* **d** is maximum time of santa's help
	
Output is in *santa.out*.

#### About

Fork process creation. Program use POSIX semaphores (without active waiting) and shared memory.

---

#### LICENSE

![CC SA](http://i.creativecommons.org/l/by-sa/3.0/88x31.png)
