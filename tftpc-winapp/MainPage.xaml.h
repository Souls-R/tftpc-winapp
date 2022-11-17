//
// MainPage.xaml.h
// MainPage 类的声明。
//

#pragma once

#include "MainPage.g.h"
namespace tftpc_winapp
{
	/// <summary>
	/// 可用于自身或导航至 Frame 内部的空白页。
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();
		//async void Invoke(Action action, Windows.UI.Core.CoreDispatcherPriority Priority = Windows.UI.Core.CoreDispatcherPriority.Normal)
		//{
		//	await Windows.ApplicationModel.Core.CoreApplication.MainView.CoreWindow.Dispatcher.RunAsync(Priority, () = > { action(); });
		//}
	private:
		void tftpcLog(Platform::String^ log);
		void tftpcLog(std::string log);
		void filePickerBtn_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void downloadBtnClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Upload_Button_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void saveFilefromBuffer(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void PageChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs e);
		void addressInputChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextBoxTextChangingEventArgs^ args);
	};
}
