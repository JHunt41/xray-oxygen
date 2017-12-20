﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace xrPostprocessEditor
{
    public partial class KeyFrameBox : UserControl
    {
        private float kf_coef;

        public event EventHandler SelectedIndexChanged;
        public event EventHandler AddButtonClick;
        public event EventHandler RemoveButtonClick;
        public event EventHandler ClearButtonClick;
        public event EventHandler KeyFrameTimeChanged;

        public ContextMenu CopyMenu { get { return btnCopyFrom.Menu; } }

        public ListBox.ObjectCollection Items { get { return lbKeyFrames.Items; } }

        public int SelectedIndex
        {
            get { return lbKeyFrames.SelectedIndex; }
            set { lbKeyFrames.SelectedIndex = value; }
        }

        public KeyFrameBox()
        {
            InitializeComponent();
            // Работаем по мотивам ПЫС, нужен коэф для названий в ListBox
            kf_coef = 0f;

            // Завезём обработчики событий
            AddButtonClick += OnAddButtonClick;
            SelectedIndexChanged += OnSelectedIndexChanged;
            RemoveButtonClick += OnRemoveButtonClick;
            ClearButtonClick += OnClearButtonClick;
            KeyFrameTimeChanged += OnKeyFrameTimeChanged;
        }
        public void OnSelectedIndexChanged(object sender, EventArgs e)
        {
            // Временно, для тестов
        }
        public void OnRemoveButtonClick(object sender, EventArgs e)
        {
            try
            {
                int size = lbKeyFrames.Items.Count - 1;
                lbKeyFrames.Items.RemoveAt(size);
                kf_coef -= 0.1f;
            }
            catch(Exception expt)
            {
                MessageBox.Show("KeyFrames is empty!");
            }
        }
        public void OnKeyFrameTimeChanged(object sender, EventArgs e)
        {

        }
        public void OnClearButtonClick(object sender, EventArgs e)
        {
            lbKeyFrames.Items.Clear();
            kf_coef = 0f;
        }

        public void OnAddButtonClick(object sender, EventArgs e)
        {
            kf_coef += 0.1f;
            lbKeyFrames.Items.Add(kf_coef.ToString());
        }

        private void lbKeyFrames_SelectedIndexChanged(object sender, EventArgs e)
        {
            SelectedIndexChanged?.Invoke(this, e);
        }

        private void btnAdd_Click(object sender, EventArgs e)
        {
            AddButtonClick?.Invoke(this, e);
        }

        private void btnRemove_Click(object sender, EventArgs e)
        {
            RemoveButtonClick?.Invoke(this, e);
        }

        private void btnClear_Click(object sender, EventArgs e)
        {
            ClearButtonClick?.Invoke(this, e);
        }

        private void numKeyFrameTime_ValueChanged(object sender, EventArgs e)
        {
            KeyFrameTimeChanged?.Invoke(this, e);
        }
    }
}
