# SP2019 Homework 2 - Bidding System
練習利用 fork 產生 process ，並用 pipe 與 FIFO 讓 process 間溝通，來實作一個小型的 bidding system 。
## Makefile
* 使用`make`產生：
	* bidding_system
	* host
	* player
	
* 使用`make clean`清除：
	* bidding_system
	* host
	* player

## bidding_system.c
* 使用函式：
	* `err_sys()`: 有錯誤時產生訊息並exit
	* `flush_sync()`: 確保寫入pipe的東西有被真正傳入
	* `find_comb`, `create_combination`: 產生所有competition的player組合
	* `calcuRank()`: 計算competition結束後的排名。  
		由於最多只有14位player，故使用暴力法解決：  
		找出未排名的player中贏最多場的次數，  
		再找哪些player符合此次數，並填入適當排名

* 變數介紹：
	* combination[][]: 存放所有competition的player組合
	* comb_num: 所有competition的player組合數
	* score[]: 存player目前得到的分數
	* cur_comb: 目前送出幾個player組合至host
	* finish: competition已經完成的個數
	* random_key: 和host_id相同，以確保不重複
	* rank[]: 存最終排名

* main內容：
	* 先創建需要的FIFO檔
	* 以nonblocking mode, read only開啟Host.FIFO  
		（不以nonblocking mode開啟會產生dead lock）
	* fork所需host，並以write only開啟Host[host_id].FIFO
	* unlink所有FIFO（之後process無預期中止後也不會有FIFO）
	* 產生所有competition的player組合存於combination[][]中
	* 將Host.FIFO設為blocking mode，這樣確保可以讀到東西
	* 適時從
		* Host[host_id].FIFO送出訊息（player_id）
		* Host.FIFO讀入訊息（key, player_id and rank）
		* 如果全部的player組合皆送出，接下來空的host會接收到`-1 -1 ...`，並且wait該host
* [取所有組合的參考資料](https://www.geeksforgeeks.org/print-all-possible-combinations-of-r-elements-in-a-given-array-of-size-n/)


## host.c
* 資料結構：
	* Data: 在root_host內使用，存放此次competition的
		* player的id
		* 此player贏了多少次
		* 此competition結束後的排名

* 使用函式：
	* `err_sys()`: 有錯誤時產生訊息並exit
	* `init_data()`: 初始化data陣列
	* `free_data()`: 釋放存data的記憶體
	* `calcuRank()`: 計算competition結束後的排名。  
		由於只有8位player，故使用暴力法解決：  
		找出未排名的player中贏最多場的次數，  
		再找哪些player符合此次數，並填入適當排名  
	* `flush_sync()`: 確保寫入pipe的東西有被真正傳入  
	* `create2children()`:
		* 產生2個child process
		* 控制自己與child process之間的pipe
		* `exec()`到對應的program

* main內容 - root_host：
	* 先產生2個child process
	* 適時從
		* Host[host_id].FIFO接收訊息（player_id）
		* Host.FIFO送出訊息（key, player_id and rank）
		* pipe_down（即write_pipe）送出訊息（player_id or winner_id）
		* pipe_up（即read_pipe）接收訊息（player_id and money）
	* 每一round皆須計算哪個player獲勝並記錄
	* 每一competition結束後皆須計算排名並傳入Host.FIFO
	* 當不使用此host時，從pipe_down（即write_pipe）送出`-1 -1 -1 -1\n`，並`wait()`2個child process及`exit(0)`

* main內容 - child_host：
	* 先產生2個child process
	* 適時從
		* STDIN_FILENO（即stdin）接收訊息（player_id or winner_id）
		* STDOUT_FILENO（即stdout）送出訊息（player_id and money）
		* pipe_down（即write_pipe）送出訊息（player_id or winner_id）
		* pipe_up（即read_pipe）接收訊息（player_id and money）
	* 當不使用此host時，從pipe_down（即write_pipe）送出`-1 -1\n`，並`wait()`2個child process及`exit(0)`

* main內容 - leaf_host：
	* 先從STDIN_FILENO（即stdin）接收player_id，再產生2個child process
	* 適時從
		* STDIN_FILENO（即stdin）接收訊息（winner_id）
		* STDOUT_FILENO（即stdout）送出訊息（player_id and money）
		* pipe_down（即write_pipe）送出訊息（winner_id）
		* pipe_up（即read_pipe）接收訊息（player_id and money）
	* 當不使用此host時，`wait()`2個child process並`exit(0)`


## player.c
* 取得arg[1]作為player_id
* money為欲付的金額，值為player_id * 100
* 使用迴圈重複10次：
	* 從stdout輸出`[player_id] [money]\n`
	* 從stdin輸入`winner_id`（除了最後一回合）
* 迴圈結束後，使用`exit(0)`結束player程式
