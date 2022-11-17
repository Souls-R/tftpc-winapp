//
// MainPage.xaml.cpp
// MainPage 类的实现。
//


#include "pch.h"
#include "MainPage.xaml.h"
#include "utils.cpp";
#include "packet.cpp"

using namespace std;
using namespace Concurrency;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::Storage::Provider;
using namespace Windows::UI::ViewManagement;
using namespace Windows::System::Threading;

using namespace tftpc_winapp;

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x804 上介绍了“空白页”项模板

//日志数组
Vector<String^>^ Logs = ref new Vector<String^>;
//下载完成的待保存文件缓冲区
map<Platform::String^, vector<Packet>> file2save;
//待上传文件缓冲区
Windows::Storage::Streams::DataReader^ uploadfileBuffer;
//待上传文件路径
String^ uploadFilepath;

//保存任务传输进度ui的引用类，对应一条任务的进度信息
class taskview {
public:
	static int totalid;
	int id;
	ProgressBar^ progressBar;
	TextBlock^ speedText;
	Button^ saveButton;
	taskview(ProgressBar^ progressBar, TextBlock^ speedText, Button^ saveButton) {
		this->id = totalid;
		totalid++;
		this->progressBar = progressBar;
		this->speedText = speedText;
		this->saveButton = saveButton;
	}
};
int taskview::totalid = 0;
vector<taskview> taskviews;

//入口函数
MainPage::MainPage()
{
	InitializeComponent();
	taskListView->Items->Clear();
	taskListView->Items->Append("等待添加任务......");
	Logs->Clear();
	//Logs->Append("等待添加任务......");
}

//日志记录函数
void tftpc_winapp::MainPage::tftpcLog(Platform::String^ log) {
	Logs->Append(log);
}
void tftpc_winapp::MainPage::tftpcLog(std::string log) {
	//需要类型转换 std::string->Platform::String^
	Logs->Append(Ts2ps(log));
}

//监听Page变动事件，仅在查看时刷新日志UI
void tftpc_winapp::MainPage::PageChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs e)
{
	switch (MainPivot->SelectedIndex)
	{
	case 0:
		LogsListView->ItemsSource = ref new Vector<String^>;
		break;
	case 1:
		LogsListView->ItemsSource = Logs;
		break;
	}
}

void tftpc_winapp::MainPage::addressInputChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextBoxTextChangingEventArgs^ args)
{

}

//调用文件选择器，选择文件进行读取，异步读取到 uploadfileBuffer
void tftpc_winapp::MainPage::filePickerBtn_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	//实例化文件选择器
	auto picker = ref new  Windows::Storage::Pickers::FileOpenPicker();
	picker->FileTypeFilter->Append(".txt");
	picker->FileTypeFilter->Append(".jpg");
	//链式异步调用
	create_task(picker->PickSingleFileAsync()).then([this](StorageFile^ file)
		{
			if (!file)return;
			create_task(file->OpenAsync(FileAccessMode::Read)).then([this, file](Windows::Storage::Streams::IRandomAccessStream^ stream)
				{
					UINT64 size = stream->Size;
					Windows::Storage::Streams::IInputStream^ inputStream = stream->GetInputStreamAt(0);
					auto dataReader = ref new Windows::Storage::Streams::DataReader(inputStream);
					create_task(dataReader->LoadAsync(size)).then([this, dataReader, file](unsigned int numBytesLoaded)
						{
							uploadfileBuffer = dataReader;
							uploadFilepath = file->Name;
							tftpcLog("选择: " + file->Name);
							//delete(dataReader);
						});
				});
		});
}

//调用文件选择器，保存文件到系统  使用sender-AccessKey作为任务标志判断保存缓冲区内的哪一个文件
void tftpc_winapp::MainPage::saveFilefromBuffer(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	auto taskid = ((Button^)sender)->AccessKey;
	auto task = file2save.find(taskid);
	if (task == file2save.end()) {
		tftpcLog("没有文件需要保存:task" + task->first);
		return;
	}
	//实例化文件选择器
	auto& filePackets = task->second;
	auto picker = ref new  Windows::Storage::Pickers::FileSavePicker();
	picker->SuggestedFileName = "filename";
	auto plainTextExtensions = ref new Platform::Collections::Vector<String^>();
	plainTextExtensions->Append(".txt");
	plainTextExtensions->Append(".jpg");
	picker->FileTypeChoices->Insert("文件类型", plainTextExtensions);
	picker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;
	//链式异步调用
	create_task(picker->PickSaveFileAsync()).then([this, filePackets](StorageFile^ file)
		{
			if (!file)return;
			create_task(file->OpenAsync(FileAccessMode::ReadWrite)).then([this, file, filePackets](Windows::Storage::Streams::IRandomAccessStream^ stream)
				{
					//auto UpdateProgress = [this](String^ speedinfo) {
					//	auto MainView = Windows::ApplicationModel::Core::CoreApplication::MainView;
					//	MainView->CoreWindow->Dispatcher->RunAsync(
					//		CoreDispatcherPriority::High,
					//		ref new DispatchedHandler([this, speedinfo]()
					//			{
					//				//speedText->Text = speedinfo;
					//			}));
					//};

					Windows::Storage::Streams::IOutputStream^ outputStream= stream->GetOutputStreamAt(0);
					auto dataWriter = ref new Windows::Storage::Streams::DataWriter(outputStream);
					for (auto i : filePackets) {
						BYTE* input = (BYTE* )i.getData();
						dataWriter->WriteBytes(ArrayReference<BYTE>(input,i.getDataLen()));

						//UpdateProgress(i.getBlockId().ToString());
					}
					dataWriter->StoreAsync();
					outputStream->FlushAsync();
				});
		});
}

//添加下载任务
void tftpc_winapp::MainPage::downloadBtnClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{	
	tftpcLog("开始下载:"+ DownloadFilePath->Text);
	int port = 69;
	auto temp = Tps2s(serverAddr->Text);
	int col = temp.find(':');
	if (col == temp.npos || col==temp.size() - 1) port = 69;
	else port = stoi(temp.substr(col + 1));
	auto ipAddress = temp.substr(0, col);
	
	auto downloadFilepath = Tps2s(DownloadFilePath->Text);
	auto downloadThreadItem = ref new WorkItemHandler(
		[this, downloadFilepath,port, ipAddress](IAsyncAction^ workItem)
		{
			//指定任务自身的id
			int taskid = taskview::totalid ;
			//更新UI界面，添加进度信息
			auto MainView = Windows::ApplicationModel::Core::CoreApplication::MainView;
			MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
				ref new DispatchedHandler([this, taskid]()
					{
						/*<StackPanel>
						<TextBlock x : Name = "speedText" Text = "等待传输"  FontSize = "16" Margin = "0,8,16,8" / >
						<StackPanel Orientation = "Horizontal" >
						<ProgressBar Value = "0" x:Name = "progressbar" HorizontalAlignment = "Left" MinWidth = "400" / >
						<Button Background = "{ThemeResource InkToolbarAccentColorThemeBrush}" Foreground = "White" Content = "保存文件" Click = "saveBtn_Click" Margin = "16,0,0,0">< / Button>
						< / StackPanel>
						< / StackPanel>*/
						auto pannel1 = ref new StackPanel;
						auto pannel2 = ref new StackPanel;
						pannel2->Orientation = Orientation::Horizontal;
						auto speedText = ref new TextBlock;
						speedText->Text = "等待传输"; speedText->FontSize = 16; speedText->Margin = Thickness(0, 8, 16, 8);
						auto progressBar = ref new ProgressBar;
						progressBar->MinWidth = 400; progressBar->Value = 0; progressBar->HorizontalAlignment = Windows::UI::Xaml::HorizontalAlignment::Left;
						auto saveButton = ref new Button;
						saveButton->Content = "保存"; saveButton->Background = ref new SolidColorBrush(Windows::UI::ColorHelper::FromArgb(0xFF, 0x6B, 0x69, 0xD6)); saveButton->Foreground = ref new SolidColorBrush(Windows::UI::Colors::White);
						saveButton->Margin = Thickness(16, 0, 0, 0); saveButton->Visibility = Windows::UI::Xaml::Visibility::Collapsed; saveButton->VerticalAlignment = Windows::UI::Xaml::VerticalAlignment::Center;
						saveButton->AccessKey = taskid.ToString();
						saveButton->Click += ref new RoutedEventHandler(this,&tftpc_winapp::MainPage::saveFilefromBuffer);
						pannel2->Children->Append(progressBar);
						pannel2->Children->Append(saveButton);
						pannel1->Children->Append(speedText);
						pannel1->Children->Append(pannel2);
						taskListView->Items->Append(pannel1);
						auto taskv = taskview(progressBar, speedText, saveButton);
						taskviews.push_back(taskv);
					}));
			//通知UI进程更新的匿名函数
			auto UpdateProgress = [this, taskid](int percent, String^ speedinfo) {
				auto MainView = Windows::ApplicationModel::Core::CoreApplication::MainView;
				MainView->CoreWindow->Dispatcher->RunAsync(
					CoreDispatcherPriority::High,
					ref new DispatchedHandler([this, taskid, percent, speedinfo]()
						{
							auto& taskv = taskviews.at(taskid);
							//未知进度，显示重复动画
							if (percent == -1) {
								taskv.progressBar->IsIndeterminate = true;
							}
							else if (percent == 100) {
								taskv.progressBar->Value = percent;
								taskv.saveButton->Visibility = Windows::UI::Xaml::Visibility::Visible;
							}
							else {
								taskv.progressBar->IsIndeterminate = false;
								taskv.progressBar->Value = percent;
							}
							taskv.speedText->Text = speedinfo;
						}));
			};
			auto tftpcLogd = [this](String^ log) {
				auto MainView = Windows::ApplicationModel::Core::CoreApplication::MainView;
				MainView->CoreWindow->Dispatcher->RunAsync(
					CoreDispatcherPriority::High,
					ref new DispatchedHandler([this, log]()
						{
							tftpcLog(log);
						}));
			};
			//启动socketAPI
			WORD wVersion = MAKEWORD(2, 2);
			WSADATA wData;
			if (WSAStartup(wVersion, &wData)) {
				tftpcLogd("启动socketAPI失败");
				return;
			}
			//启动socket连接
			SOCKET skt = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (skt == INVALID_SOCKET) {
				tftpcLogd("启动socket失败");
				return;
			}
			//设置套接字为非阻塞模式
			u_long imode = 1;
			auto ret = ioctlsocket(skt, FIONBIO, &imode);
			if (ret == SOCKET_ERROR) {
				tftpcLogd("socket参数设置失败");
				return;
			}
			//指定地址
			sockaddr_in addr{};
			addr.sin_port = htons(port);
			addr.sin_family = AF_INET;
			auto ip_char = ipAddress.c_str();
			addr.sin_addr.s_addr = inet_addr(ip_char);
			//tftpcLogd(port.ToString() + Ts2ps(ipAddress));
			//引用捕获skt,addr  带重传与超时的接收匿名函数
			auto SendwithTimeout = [&skt, &addr, tftpcLogd, this](Packet& packet, int maxRetry = 3, int timeout = 5000) {
				auto recvPkt = Packet();
				int ackid = packet.getop() == opACK ? packet.getBlockId() : 0;
				int resent = 0;
				while (resent < maxRetry) {
					sendto(skt, packet.buf, packet.len, 0, (sockaddr*)&addr, SktAddrLen);
					auto ddl = GetUnixTime() + timeout;
					while (GetUnixTime() <= ddl) {
						auto recvlen = recvfrom(skt, recvPkt.buf, BufSize, 0, (sockaddr*)&addr, &SktAddrLen);
						if (recvlen > 0) {
							recvPkt.len = recvlen;
							return recvPkt;
						}
					}
					resent++;
					tftpcLogd("ACK" + ackid + "超时重传" + resent + "次");
				}
				recvPkt.len = -1;
				tftpcLogd("ACK" + ackid + "超时发送失败");
				return recvPkt;
			};

			//RRQ请求
			auto filename = (downloadFilepath).c_str();
			auto sendPkt = Packet::RRQ(filename, Modeoctet);
			vector<Packet> receivedPackets;
			auto DATA1 = SendwithTimeout(sendPkt);
			//第一个接收包单独处理
			 if (DATA1.getop() == opDATA&& DATA1.getDataLen() < 512){//文件只有一个包需要接收
				auto ACK = Packet::ACK(1);
				sendto(skt, ACK.buf, ACK.len, 0, (sockaddr*)&addr, SktAddrLen);
				receivedPackets.push_back(DATA1);
				file2save.insert(pair<String^,vector<Packet>>(taskid.ToString(),receivedPackets));
				tftpcLogd("接收完成"); 
				UpdateProgress(100, "下载完成：" + Ts2ps(downloadFilepath) +"  总用时: 0s");
				return;
			}
			else if(DATA1.getop() == opERROR) {//错误包
				String^ errormsg = "";
				if (DATA1.getErrCode() > 0 && DATA1.getErrCode() < 8)
				{
					errormsg = "下载" + Ts2ps(downloadFilepath) + "失败 错误信息:" + Ts2ps(ErrMsg[DATA1.getErrCode()]);
				}
				else {
					errormsg = "下载" + Ts2ps(downloadFilepath) + "失败 错误信息:" + Ts2ps(DATA1.getErrMsg());
				}
				UpdateProgress(0, errormsg);
				tftpcLogd(errormsg);
				return;
			}
			else if (DATA1.len == -1) {//超时包
				String^ errormsg = "下载" + Ts2ps(downloadFilepath) + "失败 错误信息:服务器超时未响应";
				UpdateProgress(0, errormsg);
				tftpcLogd(errormsg);
				return;
			}
			
			receivedPackets.push_back(DATA1);
			//统计信息初始化
			auto startTime = GetUnixTime();
			auto lastUpdateTime = GetUnixTime();
			auto lastUpdateLengthRecv = 0;
			//DATA段接收
			const int BlockLength = 512;
			int Blockid = 1;
			while (true) {
				//组装与发送ACK
				auto ACK = Packet::ACK(Blockid);
				auto recvPkt = SendwithTimeout(ACK);
				if (recvPkt.getop() == opDATA && recvPkt.getBlockId() == Blockid + 1 && recvPkt.getDataLen() == BlockLength) {
					//tftpcLogd("下载：" + Ts2ps(downloadFilepath) + "  Block " + recvPkt.getBlockId() + "recv");
					receivedPackets.push_back(recvPkt);
					auto now_lengthrecv = receivedPackets.size() * BlockLength;
					//速度信息  每0.5s更新一次
					auto nowTime = GetUnixTime();
					if (nowTime > lastUpdateTime + 500) {
						double speed = (now_lengthrecv - lastUpdateLengthRecv) / (nowTime - lastUpdateTime);
						speed = (speed * 1000) / 1024;//kb/s
						String^ speedinfo ="下载："+ Ts2ps(downloadFilepath) + " " +now_lengthrecv.ToString() + "bytes已接收   速度:" + itos_in2(speed) + "kb/s";
						UpdateProgress(-1, speedinfo);
						lastUpdateTime = nowTime;
						lastUpdateLengthRecv = now_lengthrecv;
					}
				}
				//已收到的服务器重传包，丢弃
				else if (recvPkt.getop() == opDATA && recvPkt.getBlockId() < Blockid + 1 && recvPkt.getDataLen() == BlockLength) {
					tftpcLogd("下载：" + Ts2ps(downloadFilepath) + "  Block " + recvPkt.getBlockId() + "重复接收");
					continue;
				}
				//最后一个包
				else if (recvPkt.getop() == opDATA && recvPkt.getBlockId() == Blockid + 1 && recvPkt.getDataLen() < BlockLength) {
					receivedPackets.push_back(recvPkt);
					auto ACK = Packet::ACK(Blockid + 1);
					sendto(skt, ACK.buf, ACK.len, 0, (sockaddr*)&addr, SktAddrLen);
					file2save.insert(pair<String^, vector<Packet>>(taskid.ToString(), receivedPackets));
					tftpcLogd("接收完成");
					break;
				}
				//错误包处理
				else if(recvPkt.getop() == opERROR) {
					String^ errormsg = "";
					if (recvPkt.getErrCode() > 0 && recvPkt.getErrCode() < 8)
					{
						errormsg = "下载" + Ts2ps(downloadFilepath) + "失败 错误信息:" + Ts2ps(ErrMsg[recvPkt.getErrCode()]);
					}
					else {
						errormsg = "下载" + Ts2ps(downloadFilepath) + "失败 错误信息:" + Ts2ps(recvPkt.getErrMsg());
					}
					UpdateProgress(0, errormsg);
					tftpcLogd(errormsg);
					return;
				}
				//其余情况视为错误
				else {
					tftpcLogd("Block" + (Blockid + 1) + "接收失败");
					UpdateProgress(0, "下载" + Ts2ps(downloadFilepath) + "失败");
					return;
				}
				//下一个包
				Blockid++;
			}
			//结束传输，最终的统计信息
			auto endTime = GetUnixTime();
			double totalTime = (static_cast<double>(endTime) - startTime) / 1000;
			double speed = (receivedPackets.size() * BlockLength) / (static_cast<double>(endTime) - startTime + 1);//防止除以0
			String^ successmsg = "下载完成：" + Ts2ps(downloadFilepath) + " " + (receivedPackets.size() * BlockLength).ToString() + "bytes"
				+ " 速度:" + itos_in2(speed) + "kb/s" + "  总用时:" + itos_in2(totalTime) + " s";
			tftpcLog(successmsg);
			UpdateProgress(100, successmsg);

		});
	//启动新线程
	auto asyncAction = ThreadPool::RunAsync(downloadThreadItem);

}

//添加上传任务
void tftpc_winapp::MainPage::Upload_Button_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (!uploadfileBuffer) {
		tftpcLog("未选择文件");
		return;
	}
	int port = 69;
	auto temp = Tps2s(serverAddr->Text);
	int col = temp.find(':');
	if (col == temp.npos || col == temp.size() - 1) port = 69;
	else port = stoi(temp.substr(col + 1));
	auto ipAddress = temp.substr(0, col);

	auto uploadfileBuffer_temp =  &uploadfileBuffer;   //uploadfileBuffer无法直接捕获进线程
	auto uploadFilepath_temp =  &uploadFilepath;   //uploadFilepath无法直接捕获进线程
	auto uploadThreadItem = ref new WorkItemHandler(
		[this, uploadfileBuffer_temp, uploadFilepath_temp, port, ipAddress](IAsyncAction^ workItem)
		{
			auto uploadfileBuffer = (*uploadfileBuffer_temp);
			auto uploadFilepath_s = Tps2s(*uploadFilepath_temp);
			const char* uploadFilepath_char = uploadFilepath_s.c_str();
			//指定自身id
			int taskid = taskview::totalid;
			//更新UI界面，添加任务UI进入列表
			auto MainView = Windows::ApplicationModel::Core::CoreApplication::MainView;
			MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
				ref new DispatchedHandler([this]()
					{
						/*<StackPanel>
						<TextBlock x : Name = "speedText" Text = "等待传输"  FontSize = "16" Margin = "0,8,16,8" / >
						<StackPanel Orientation = "Horizontal" >
						<ProgressBar Value = "0" x:Name = "progressbar" HorizontalAlignment = "Left" MinWidth = "400" / >
						<Button Background = "{ThemeResource InkToolbarAccentColorThemeBrush}" Foreground = "White" Content = "保存文件" Click = "saveBtn_Click" Margin = "16,0,0,0">< / Button>
						< / StackPanel>
						< / StackPanel>*/
						auto pannel1 = ref new StackPanel;
						auto pannel2 = ref new StackPanel;
						pannel2->Orientation = Orientation::Horizontal;
						auto speedText = ref new TextBlock;
						speedText->Text = "等待传输"; speedText->FontSize = 16; speedText->Margin = Thickness(0, 8, 16, 8);
						auto progressBar = ref new ProgressBar;
						progressBar->MinWidth = 400; progressBar->Value = 0; progressBar->HorizontalAlignment = Windows::UI::Xaml::HorizontalAlignment::Left;
						auto saveButton = ref new Button;
						pannel2->Children->Append(progressBar);
						pannel1->Children->Append(speedText);
						pannel1->Children->Append(pannel2);
						taskListView->Items->Append(pannel1);
						auto taskv = taskview(progressBar, speedText, saveButton);
						taskviews.push_back(taskv);
					}));
			//通知UI进程更新的匿名函数
			auto UpdateProgress = [this, taskid](int percent, String^ speedinfo) {
				auto MainView = Windows::ApplicationModel::Core::CoreApplication::MainView;
				MainView->CoreWindow->Dispatcher->RunAsync(
					CoreDispatcherPriority::High,
					ref new DispatchedHandler([this, taskid, percent, speedinfo]()
						{
							auto& taskv = taskviews.at(taskid);
							if (percent == -1) {
								taskv.progressBar->IsIndeterminate = true;
							}
							else if (percent == 100) {
								taskv.progressBar->Value = percent;
							}
							else {
								taskv.progressBar->IsIndeterminate = false;
								taskv.progressBar->Value = percent;
							}
							taskv.speedText->Text = speedinfo;

						}));
			};
			auto tftpcLogd = [this](String^ log) {
				auto MainView = Windows::ApplicationModel::Core::CoreApplication::MainView;
				MainView->CoreWindow->Dispatcher->RunAsync(
					CoreDispatcherPriority::High,
					ref new DispatchedHandler([this, log]()
						{
							tftpcLog(log);
						}));
			};
			//启动socketAPI
			WORD wVersion = MAKEWORD(2, 2);
			WSADATA wData;
			if (WSAStartup(wVersion, &wData)) {
				tftpcLogd("启动socketAPI失败");
				return;
			}
			//启动socket连接
			SOCKET skt = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (skt == INVALID_SOCKET) {
				tftpcLogd("启动socket失败");
				return;
			}
			//设置套接字为非阻塞模式
			u_long imode = 1;
			auto ret = ioctlsocket(skt, FIONBIO, &imode);
			if (ret == SOCKET_ERROR) {
				tftpcLogd("socket参数设置失败");
				return;
			}
			//指定地址
			sockaddr_in addr{};
			addr.sin_port = htons(port);
			addr.sin_family = AF_INET;
			auto ip_char = ipAddress.c_str();
			addr.sin_addr.s_addr = inet_addr(ip_char);
			//引用捕获skt,addr  带重传与超时的发送
			auto SendwithTimeout = [&skt, &addr, tftpcLogd, this](Packet& packet, int maxRetry = 3, int timeout = 5000) {
				auto recvPkt = Packet();
				int blockid = packet.getop() == opDATA ? packet.getBlockId() : 0;
				int resent = 0;
				while (resent <= maxRetry) {
					sendto(skt, packet.buf, packet.len, 0, (sockaddr*)&addr, SktAddrLen);
					auto ddl = GetUnixTime() + timeout;
					while (GetUnixTime() <= ddl) {
						auto recvlen = recvfrom(skt, recvPkt.buf, BufSize, 0, (sockaddr*)&addr, &SktAddrLen);
						if (recvlen > 0) {
							recvPkt.len = recvlen;
							return recvPkt;
						}
					}
					resent++;
					tftpcLogd("Block" + blockid + "超时重传" + resent + "次");
				}
				recvPkt.len = -1;
				tftpcLogd("Block" + blockid + "超时发送失败");
				return recvPkt;
			};

			//WRQ请求
			const char* filename= uploadFilepath_char;
			auto packet2send = Packet::WRQ(filename, Modeoctet);
			auto recvPkt=SendwithTimeout(packet2send);

			//统计信息初始化
			unsigned int percent = 0;
			auto fileLength = uploadfileBuffer->UnconsumedBufferLength;
			auto startTime = GetUnixTime();
			auto lastUpdateTime = GetUnixTime();
			auto lastUpdateLengthSent=0;
			//DATA段发送
			const int BlockLength = 512;
			int Blockid = 1;
			while (uploadfileBuffer->UnconsumedBufferLength > 0) {
				//检查
				if (recvPkt.getop() == opACK && packet2send.getop() == opWRQ) {

					//继续
				}
				else if (recvPkt.getop() == opACK && recvPkt.getBlockId() == packet2send.getBlockId()) {
					
					tftpcLogd("上传：" + Ts2ps(filename) + "  Block " + recvPkt.getBlockId() + "sent");
					//速度信息  每1%进度更新一次
					auto now_percent = 100 - static_cast<unsigned int>(100.f * uploadfileBuffer->UnconsumedBufferLength / fileLength);
					if (now_percent != percent) {
						auto nowTime = GetUnixTime();
						percent = now_percent;
						auto now_lengthsent = fileLength - uploadfileBuffer->UnconsumedBufferLength;
						double speed = (now_lengthsent - lastUpdateLengthSent) / (nowTime - lastUpdateTime + 1);//防止除以0
						speed = (speed * 1000) / 1024;//kb/s
						double eta = (uploadfileBuffer->UnconsumedBufferLength / speed) / 1000;//s
						String^ speedinfo = "上传：" + Ts2ps(filename) + " " + now_lengthsent.ToString() + "bytes / " + fileLength.ToString()
							+ "bytes  速度:" + itos_in2(speed) + "kb/s" + "  预计等待:" + itos_in2(eta) + " s";
						UpdateProgress(percent, speedinfo);
						lastUpdateTime = nowTime;
						lastUpdateLengthSent = now_lengthsent;
					}
				}
				//已接收的ACK，重传当前包
				else if (recvPkt.getop() == opACK && recvPkt.getBlockId() < packet2send.getBlockId()) {
					tftpcLogd("上传：" + Ts2ps(filename) + "  ACK " + recvPkt.getBlockId() + "重复接收");
					recvPkt = SendwithTimeout(packet2send);
					continue;
				}
				else if (recvPkt.getop() == opERROR) {//错误包
					String^ errormsg = "";
					if (recvPkt.getErrCode() > 0 && recvPkt.getErrCode() < 8)
					{
						errormsg = "上传" + Ts2ps(filename) + "失败 错误信息:" + Ts2ps(ErrMsg[recvPkt.getErrCode()]) + "Block" + (Blockid + 1) + "发送失败";
					}
					else {
						errormsg = "上传" + Ts2ps(filename) + "失败 错误信息:" + Ts2ps(recvPkt.getErrMsg()) + "Block" + (Blockid + 1) + "发送失败";
					}
					UpdateProgress(0, errormsg);
					tftpcLogd(errormsg);
					return;
				}
				else {//超时包等视为错误
					tftpcLogd("Block" + Blockid + "发送失败");
					UpdateProgress(0, "上传" + Ts2ps(filename) + "失败");
					return;
				}
				

				BYTE fileBlock[BlockLength]{};
				//分包
				auto packetLength = min(uploadfileBuffer->UnconsumedBufferLength, BlockLength);
				uploadfileBuffer->ReadBytes(ArrayReference<BYTE>(fileBlock, packetLength));
				packet2send = Packet::DATA(Blockid, (char*)fileBlock, packetLength);
				//发送
				recvPkt = SendwithTimeout(packet2send);
				//下一个包
				Blockid++;
			}
			//上传结束，最终的统计信息
			auto endTime = GetUnixTime();
			double totalTime = (static_cast<double>(endTime) - startTime) / 1000;
			double speed = (fileLength) / (static_cast<double>(endTime) - startTime + 1);//防止除以0
			String^ successmsg = "上传完成：" + Ts2ps(filename) + " " + fileLength.ToString() + "bytes / " + fileLength.ToString()
				+ "bytes  速度:" + itos_in2(speed) + "kb/s" + "  总用时:" + itos_in2(totalTime) + " s";
			tftpcLogd(successmsg);
			UpdateProgress(100,successmsg);

		});
	//启动新线程
	auto asyncAction = ThreadPool::RunAsync(uploadThreadItem);
	//auto m_workItem = asyncAction;
	//asyncAction->Completed = ref new AsyncActionCompletedHandler(
	//	[this](IAsyncAction^ asyncInfo, AsyncStatus asyncStatus)
	//	{
	//		// Update the UI thread with the CoreDispatcher.
	//		auto MainView = Windows::ApplicationModel::Core::CoreApplication::MainView;
	//		MainView->CoreWindow->Dispatcher->RunAsync(
	//			Windows::UI::Core::CoreDispatcherPriority::High,
	//			ref new Windows::UI::Core::DispatchedHandler([this]()
	//				{
	//					tftpcLog("处理完成");
	//				}));
	//	});

}




