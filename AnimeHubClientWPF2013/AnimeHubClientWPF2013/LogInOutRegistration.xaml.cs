using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace AnimeHubClientWPF2013
{
    public partial class LogInOutRegistration : Page
    {
        public LogInOutRegistration()
        {
            InitializeComponent();
            if (App.LoggedIn)
            {
                AlreadyLoggedStack.Visibility = Visibility.Visible;
                LogInStack.Visibility = Visibility.Hidden;
                RegistrationStack.Visibility = Visibility.Hidden;
                AlreadyLoggedTextBox.Text = "Вы вошли как " + App.UserName;
                ServerAnsverTextBlock.Text = "";
            }
            else
            {
                AlreadyLoggedStack.Visibility = Visibility.Hidden;
                LogInStack.Visibility = Visibility.Visible;
                RegistrationStack.Visibility = Visibility.Hidden;
                AlreadyLoggedTextBox.Text = "";
                ServerAnsverTextBlock.Text = "";
            }
        }
        private unsafe void LogInButtonClick(object sender, EventArgs e)
        {
            char[] login = (LogInTextBox.Text + '\0').ToCharArray();
            char[] password = (LogInPasswordBox.Password + '\0').ToCharArray();
            App.LoggedIn = App.LogIn(login, password);
            if (App.LoggedIn)
            {
                App.UserName = LogInTextBox.Text;
                AlreadyLoggedTextBox.Text = "Вы вошли как " + App.UserName;
                AlreadyLoggedStack.Visibility = Visibility.Visible;
                ServerAnsverTextBlock.Text = "";
                LogInStack.Visibility = Visibility.Hidden;

            }
            else
            {
                ServerAnsverTextBlock.Text = "Неправильный логин или пароль";
            }
        }
        private void GoToRegistrationButtonClick(object sender, EventArgs e)
        {
            ServerAnsverTextBlock.Text = "";
            AlreadyLoggedTextBox.Text = "";
            RegistrationStack.Visibility = Visibility.Visible;
            LogInStack.Visibility = Visibility.Hidden;
        }
        private unsafe void RegistrationButtonClick(object sender, EventArgs e)
        {
            char[] newlogin = (RegistrationTextBox.Text + '\0').ToCharArray();
            char[] newpassword = (RegistrationPasswordBox.Password + '\0').ToCharArray();
            bool is_registred = false;
            is_registred = App.Register(newlogin, newpassword);
            if (is_registred)
            {
                AlreadyLoggedTextBox.Text = "";
                RegistrationStack.Visibility = Visibility.Hidden;
                ServerAnsverTextBlock.Text = "Вы успешно зарегистрировали новый аккаунт, теперь пойдите в него";
                LogInStack.Visibility = Visibility.Visible;

            }
            else
            {
                ServerAnsverTextBlock.Text = "Не получилось зарегистрировать аккаунт, возможно аккаунт с таким именем уже существует";
            }
        }
        private void RegistrationCancelButtonClick(object sender, EventArgs e)
        {
            ServerAnsverTextBlock.Text = "";
            AlreadyLoggedTextBox.Text = "";
            RegistrationStack.Visibility = Visibility.Hidden;
            LogInStack.Visibility = Visibility.Visible;
        }
        private void LogOutButtonClick(object sender, EventArgs e)
        {
            App.LogOut();
            App.LoggedIn = false;
            AlreadyLoggedStack.Visibility = Visibility.Hidden;
            ServerAnsverTextBlock.Text = "";
            LogInStack.Visibility = Visibility.Visible;
            AlreadyLoggedTextBox.Text = "";
        }
    }
}
