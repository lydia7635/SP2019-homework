## How to Run My Code
* 使用`make`產生：
	* hw4

* 使用`make debug`產生：
	* 可看每次iteration時間的hw4

* 使用`make create`產生：
	* 可看train資料的圖片大概長相  
	（使用`./createImage [index]`，編號從1開始）  
	（也可把程式中train改成test，查看test資料的圖片）

* 使用`make comparing`產生：
	* 比對result.csv和y_test的程式  
	（使用`./compare [number]`，number為需比對的資料量）

* 使用`make clean`清除：
	* hw4
	* createImage
	* compare

## 使用函式
* `err_sys()`: 有錯誤時產生訊息並exit
* `readX()`: 讀入X_train/X_test資料，並將資料normalize後存入`trainX/testX`
* `readY()`: 讀入y_train，並將此轉為one-hot存入`trainY`
* `multiXW()`: 將`trainX/testX`和`W`矩陣相乘，並存入`trainZ/testZ`中
	* 對應公式(1)
* `softmax()`: 將`trainZ/testZ`經由softmax函數轉換存入`trainY_hat/testY_hat`
	* 對應公式(2)
* `mltiXY()`: 對`trainX`、`trainY`、`trainY_hat`做處理，並存入`W_grad`中
	* 對應公式(4)
* `refreshW()`: 使用`W_grad`更新`W`
	* 對應公式(3)
* `pr_times()`: 印出從開始到各階段經過的real/user/sys time

## main內容
* 取得開始時的絕對時間並存入（debug）
* 讀入所需資料X_train、y_train
	* tip: 盡量別一筆一筆資料讀入，讀硬碟資料很花時間
* 每次iteration需做下列事項：
	* 分出所需個thread並讓他們執行`multiXW()`
	* join thread，避免其成為zombie，並保證thread已完成XW相乘
	* `softmax()`
	* `multiXY`
	* `refreshW()`
	* 取得此時絕對時間，並印出從開始到現在執行了多久（debug）
	* tip: 以陣列做矩陣相乘時，盡量讓記憶體為「連續存取」，以增加效率  
	意即`arr[i][j]`中的`j`為內層迴圈變數
* 讀入X_test，並經由公式(1)、(2)產生testY_hat
* 將每筆資料中，選擇有最大可能性的作為預測label，並將此寫入result.csv中
	* 依照`"id,label\n"`、`"%d,%d\n", id, label`的格式寫入