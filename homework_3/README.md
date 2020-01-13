## How to Run My Code
* 使用`make`產生：
	* main
	* hw3
	
* 使用`make clean`清除：
	* main
	* hw3


## main.c
* 使用函式：
	* `err_sys()`: 有錯誤時產生訊息並exit
	* `createChild()`: `fork`一個child process並執行hw3

* 變數介紹：
	* `sig[]`: 存什麼時間該傳哪種signal
	* `cpid`: child process id
	* `ACK[]`: 儲存從pipe收到的ACK
	* `arr[]`: 儲存從pipe收到的hw3之arr

* main內容：
	* stdin讀入所需資料
	* 產生child process並執行hw3
	* 一開始5秒後依照`sig[]`發送對應之signal，  
	之後等到成功接收ACK後再發送下一個signal  
	（如果發出SIGUSR3，接收ACK後要印出其調整後的內容至stdout）
	* 全部signal發送完畢後，接收ACK並印出至stdout（此時內容為hw3之arr）
	* `wait()` child process


## hw3.c
* 使用函式：
	* `err_sys()`: 有錯誤時產生訊息並exit
	* `sigHandler()`: `SIGUSR1/2/3`之signal handler
	* `make_circular_linked_list()`: 做circular linked list
	* `getLock()`: 取得arr lock，取得成功回傳true，否則回傳false
	* `releaseLock()`: 釋放arr lock
	* `funct_x()`: 因為`funct_1/2/3/4`有極大部分重複，故使用此函式

* 變數介紹：
	* `MAIN`: jmp_buf，用於從`funct_4()`跳回`main()`
	* `P`, `Q`, `task`, `runTime`: 對應hw3傳入`main()`之argument
	* `newmask`: 可以block `SIGUSR1/2/3`之mask
	* `oldmask`: 原來的mask
	* `pendmask`: 用於記錄哪些signal被pending中
	* `unmask1/2/3`: 可以用來unblock特定signal之mask
	* `queue[]`: 記錄哪些function在queue中

* hw3 - main內容：
	* 將`SIGUSR1/2/3`設為catch，由`sigHandler()`處理
	* 將此三個signal設為block
	* 由`funct_5()`開始，依照spec的順序呼叫函式或`longjmp()`
	* 處理FCB circular linked list
	* 開始依照FCB list的順序處理function

* hw3 - funct_x內容：
	* 處理FCB circular linked list，用`setjmp()`將目前狀態存入`Environment`
	* 如果由`setjmp()`跳回，呼叫函式或`longjmp()`
	* 每次執行大迴圈，開頭都要加`sigprocmask()`，  
	避免從`sigHandler()`跳回時mask會設置成前一次呼叫`sigprocmask()`要求的mask  
	（`sigprocmask()`會把pending且未block的signal deliver後，  
	處理完再return）
	* 如果無法成功取得arr lock，  
	將目前狀態存入`Environment`後跳至下一個函式，	並將它放入queue
	* 如果成功取得arr lock，將此函式從queue中刪除，  
	並執行小迴圈將對應字元放入arr
	* 每次完全執行完小迴圈，依照task執行以下事項：
		- task 2: 依照`runTime`要求，在符合的時間點釋放arr lock後，  
		將目前狀態存入`Environment`並跳至下一個函式
		- task 3: 查看是否有signal被pending並unblock它。  
		如果pending的為SIGUSR2，先釋放arr lock後再unblock
	* 大迴圈會跑P + 1次（為了在填完arr再回來後仍能擁有lock），  
	第P + 1次在進入小迴圈前會用break跳出大迴圈
	* 完全執行完大迴圈後釋放arr lock，並`longjmp()`至`Scheduler()`

* hw3 - sigHandler內容：
	* 先將signal mask還原成可以block三個signal的狀態
	* 根據收到的signal類型，將相對應的資料輸出至stdout，  
	並調整Current指到的FCB block，  
	使之後可由`Schedule()`跳到對應的function

## 問題
* spec內的`void funct_1(int name)`有`name`參數，但是scheduler.h內沒有
	- 有參數為準，而且有加也不會錯誤

* `dummy(int name)`: name的意義？
	- 知道下一個要叫誰

* 未進入`Scheduler()`前，從`funct_1/2/3`跳回`dummy()`是使用`longjmp()`還是函式`return`？  
（因為stackframe不是要在不一樣的位置？）
	- 是呼叫函式

* 因為report.pdf要提到如何實作，所以這次readme只寫如何執行自己的code嗎？
	- 有交report: 是；不交: 要附上如何實作
	
* report有規定的格式或檔名嗎？
	- 限制一頁，檔名：report.pdf

* SIGUSR2: release lock是否要在signal handler內做？
	- 否，在unblock signal前做

* 在task 3，funct_x中while(1) 和 put funct_x into the queue 之用處？
	- NULL

* 如果大迴圈結束同時收到訊號，先後順序？
	- 先deliver訊號，之後要得到lock才可繼續

* 需要印出child switch to funct x嗎？
	- 助教測試時會關掉

* 	10 3  
	10  
	1 3 1 3 2 3 1 3 2 2的答案？
	- 2 3 4
	  2 3 4  
	  3 4  
	  1 3 4  
	  (30個1)(24個2)(6個3)(30個4)(6個2)(24個3)

## 備註
* schedule.c為展示schedule.o流程大致為何，但非完全相符（例如output出提示）
