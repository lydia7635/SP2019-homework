# SP2019 Homework 1 - Banking System
利用 I/O multiplexing 與 file lock 實作一個簡單的 banking system 。

## Usage
使用 `make` 產生 write_server 與 read_server。
* server side （假設在工作站 linux1 上）：執行 read_server 與 write_server
	* (one terminal) `./read_server <port>`
	* (another terminal) `./write_server <port2>`
* client side：透過 telnet 連接到 read_server 或 write_server
	`telnet 140.112.30.32 <port/port2>`
	* 執行指令請參考 Problem description


## Other Tools
使用 `make account` 生成執行檔
* check_account
	- 查看帳戶餘額
* init_account
	- 重設所有帳戶（所有金額設為 3000）

## 詳細做法
為保險起見，用 fcntl 設 svr.listen_fd 為 nonblocking mode

Multiplexing tasks:
original_set 代表需要檢視的 file descriptor set
working_set 先複製一份 original_set 再運作

首先須將 svr.listen_fd 位置的 original_set 設為 on
如果有新連線（不一定有資料），select 後此位不會被設為 off
有新連線：
	accept 此連線，並將回傳之 conn_fd 位置的 original_set 設為 on
	並重頭開始 while 迴圈（ continue ）
連線內有資料：
	select 後有資料的位置不會被設為 off
	用for迴圈搜尋此位，並繼續之後的作業
	工作完後記得將 original_set 中的此位置 off

file_fd 指向名為 account_list 的檔案
(<0 即代表無此檔案)


使用 fcntl 控制檔案鎖以及資料處理：
requestP[conn_fd].item 存入該連線需查看或更改的帳戶
如果連線至 read_server 內
檢查該帳戶資料處是否可以上 read lock^[註1]
如果不行，退出此次連線
如果可以，該帳戶資料處上 read lock
再從 account_list 以 read() 讀入帳戶資料
並 write() 進連線進來的 client 中
最後要把此 read lock 解除並退出此次連線

如果連線至 write_server 內
有兩種狀況：讀入需更改帳戶或操作方法
(1) 讀入需更改帳戶
	此時 requestP[conn_fd].item 為 0
	atoi(requestP[conn_fd].buf) 會產生 >0 的數
	（如果是合法帳戶，範圍為1~20）

	將此值存入 requestP[conn_fd].item 供下次取用
	接下來檢查該帳戶資料處是否可以上 write lock^[註2]
	如果不行，退出此次連線
	如果可以，該帳戶資料處上 write lock
	並使用 continue 繼續 multiplexing tasks

(2) 讀入操作方法
	此時 requestP[conn_fd].item 不為 0

	接下來依照操作方法： save, withdraw, transfer, balance 來修改檔案
	(a) save: 讀到 requestP[conn_fd].buf 的首字為 s
	(b) withdraw: 讀到 requestP[conn_fd].buf 的首字為 w
	(c) transfer: 讀到 requestP[conn_fd].buf 的首字為 t
	(d) balance: 讀到 requestP[conn_fd].buf 的首字為 b
		（為了怕和變數名稱重複，此函式名為 Balance ）
	從 requestP[conn_fd].buf 以 sscanf() 讀入所需資料
	從 account_list 以 read() 讀入所需資料
	其中 transfer 比較特別，需讀入轉出帳戶及轉入帳戶之存款
	將修改後的資料寫回 account_list
	最後要把此 write lock 解除並退出此次連線


[註1] 是否可以上 read lock 需滿足兩個條件
	  (1) 同一個 server 內沒有在此上 write lock ： writeLock[] 陣列會記錄
		（因為同一個 process 內的鎖是相容的）
	  (2) 用 fcntl 上 read lock 可以成功（返回值不為-1）

	  成功用 fcntl 上 read lock 時，需在對應帳戶的 readLock[] 內設 true

	  解鎖用 fcntl 上的 read lock 時，需在對應帳戶的 readLock[] 內設 false

[註2] 是否可以上 write lock 需滿足兩個條件
	  (1) 同一個 server 內沒有在此上 write lock 及 read lock： writeLock[] 及 readLock[] 陣列會記錄
		（因為同一個 process 內的鎖是相容的）
	  (2) 用 fcntl 上 write lock 可以成功（返回值不為-1）

	  成功用 fcntl 上 write lock 時，需在對應帳戶的 writeLock[] 內設 true

	  解鎖用 fcntl 上的 write lock 時，需在對應帳戶的 writeLock[] 內設 false