﻿using ResourceTool.Utils;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace ResourceTool.ViewModel.Dialog
{
    public class CreateResourceBaseDialogViewModel : DialogViewModel
    {
        public override string Title { get { return "Create resource base"; } }

        private string _name = string.Empty;
        public string Name
        {
            get { return _name; }
            set
            {
                if (value == _name)
                {
                    return;
                }

                _name = value;
                OnPropertyChanged(nameof(Name));
                CommandManager.InvalidateRequerySuggested();
            }
        }

        private string _path = string.Empty;
        public string Path
        {
            get { return _path; }
            set
            {
                if (value == _path)
                {
                    return;
                }

                _path = value;
                OnPropertyChanged(nameof(Path));
                CommandManager.InvalidateRequerySuggested();
            }
        }

        public CreateResourceBaseDialogViewModel()
        {

        }

        protected override bool OkCommandCanExecute(object parameter)
        {
            return
                base.OkCommandCanExecute(parameter) &&
                !string.IsNullOrEmpty(Name) &&
                Directory.Exists(Path);
        }
    }
}
