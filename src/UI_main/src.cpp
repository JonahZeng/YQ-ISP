/*
 * @Author: Jonah Zeng zengyangqiao@qq.com
 * @Date: 2022-06-22 16:41:44
 * @LastEditors: Jonah Zeng zengyangqiao@qq.com
 * @LastEditTime: 2022-06-27 14:59:31
 * @FilePath: \hello_world\src.cpp
 * @Description: 
 * 
 * Copyright (c) 2022 by Jonah Zeng zengyangqiao@qq.com, All Rights Reserved. 
 */
#include <wx/wxprec.h>
#include <wx/filesys.h>
 
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/statline.h>
#ifndef _MSC_VER
#include "human_brain.xpm"
#endif

#include "isp_interface.h"


wxDECLARE_EVENT(wxEVT_COMMAND_MYTHREAD_COMPLETED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_COMMAND_MYTHREAD_UPDATE, wxCommandEvent);
class mainFrame;

class MyThread : public wxThread
{
public:
    MyThread(mainFrame* handler, int pipeId, wxString logFilePath, wxString xmlPath, int frameCnt)
        :wxThread(wxTHREAD_DETACHED), pipe_id(pipeId), log_file_path(logFilePath), xml_path(xmlPath), frame_cnt(frameCnt),
        m_pHandler(handler)
    {

    }
    ~MyThread();

protected:
    virtual ExitCode Entry();
    mainFrame *m_pHandler;
private:
    int pipe_id;
    wxString log_file_path;
    wxString xml_path;
    int frame_cnt;
};


class mainFrame : public wxFrame
{
public:
    mainFrame(const wxString& title);
    void OnRunClick(wxCommandEvent& event);
    void OnSelectXmlClick(wxCommandEvent& event);
    void OnSelectLogClick(wxCommandEvent& event);
    void OnThreadUpdate(wxCommandEvent& event);
    void OnThreadCompletion(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

private:
    wxTextCtrl* cfgFile_path;
    wxTextCtrl* logFile_path;
    wxComboBox* pipeID_list;
    wxComboBox* frameCnt_list;
    wxStaticText* isp_status;
    wxBoxSizer* horLayout;
    wxBoxSizer* verLayout;
protected:
    MyThread *m_pThread;
    wxCriticalSection m_pThreadCS;    // protects the m_pThread pointer
    friend class MyThread;  
    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(mainFrame, wxFrame)
    EVT_CLOSE(mainFrame::OnClose)
    EVT_COMMAND(wxID_ANY, wxEVT_COMMAND_MYTHREAD_UPDATE, mainFrame::OnThreadUpdate)
    EVT_COMMAND(wxID_ANY, wxEVT_COMMAND_MYTHREAD_COMPLETED, mainFrame::OnThreadCompletion)
wxEND_EVENT_TABLE()

wxDEFINE_EVENT(wxEVT_COMMAND_MYTHREAD_COMPLETED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_MYTHREAD_UPDATE, wxCommandEvent);


wxThread::ExitCode MyThread::Entry()
{
    //while (!TestDestroy())
    //{
        // ... do a bit of work...
 
       // wxQueueEvent(m_pHandler, new wxThreadEvent(wxEVT_COMMAND_MYTHREAD_UPDATE));
    //}
 
    // signal the event handler that this thread is going to be destroyed
    // NOTE: here we assume that using the m_pHandler pointer is safe,
    //       (in this case this is assured by the MyFrame destructor)
    bool ret = false;
    if(!wxFileName::FileExists(log_file_path) || !wxFileName::FileExists(xml_path))
    {

    }
    else
    {
        ret = run_isp(pipe_id, log_file_path.c_str(), xml_path.c_str(), frame_cnt);
    }

    wxCommandEvent* compelete_event = new wxCommandEvent(wxEVT_COMMAND_MYTHREAD_COMPLETED);
    compelete_event->SetInt(ret);
    wxQueueEvent(m_pHandler, compelete_event);
 
    return (wxThread::ExitCode)0;     // success
}

MyThread::~MyThread()
{
    wxCriticalSectionLocker enter(m_pHandler->m_pThreadCS);
 
    // the thread is being destroyed; make sure not to leave dangling pointers around
    m_pHandler->m_pThread = nullptr;
}



enum {
    ID_PIPE_ID_COMBOX = 1,
    ID_FRAME_CNT_COMBOX = 2,
    ID_CFG_FILE_TEXT = 3,
    ID_CFG_FILE_BUTTON = 4,
    ID_LOG_FILE_TEXT = 5,
    ID_LOG_FILE_BUTTON = 6,
    ID_RUN_BUTTON = 7,
    ID_ISP_STATUS = 8
};

mainFrame::mainFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(800, 480)), m_pThread(nullptr)
{
#ifndef _MSC_VER
    SetIcon(wxIcon(aaa));
#endif
    wxPanel* mainPanel = new wxPanel(this);
    verLayout = new wxBoxSizer(wxVERTICAL);
    horLayout = new wxBoxSizer(wxHORIZONTAL);

    wxStaticText* pipeID_text = new wxStaticText(mainPanel, wxID_ANY, wxString("pipe ID:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    wxString pipe_id_strings[3] = {"0", "1", "2"};
    pipeID_list = new wxComboBox(mainPanel, ID_PIPE_ID_COMBOX, wxString(""), wxDefaultPosition, wxDefaultSize, 3, pipe_id_strings);

    wxStaticText* frameCnt_text = new wxStaticText(mainPanel, wxID_ANY, wxString("frame cnt:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    wxString frame_cnt_strings[4] = {"1", "2", "3", "4"};
    frameCnt_list = new wxComboBox(mainPanel, ID_FRAME_CNT_COMBOX,  wxString(""), wxDefaultPosition, wxDefaultSize, 4, frame_cnt_strings);

    cfgFile_path = new wxTextCtrl(mainPanel, ID_CFG_FILE_TEXT, wxString(wxT("")));
    wxButton* cfgFile_button = new wxButton(mainPanel, ID_CFG_FILE_BUTTON, wxString("select xml"));

    logFile_path = new wxTextCtrl(mainPanel, ID_LOG_FILE_TEXT, wxString(wxT("")));
    wxButton* logFile_button = new wxButton(mainPanel, ID_LOG_FILE_BUTTON, wxString("select log"));

    horLayout->Add(pipeID_text, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    horLayout->Add(pipeID_list, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    horLayout->Add(frameCnt_text, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    horLayout->Add(frameCnt_list, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    horLayout->AddStretchSpacer();
    horLayout->Add(cfgFile_path, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    horLayout->Add(cfgFile_button, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    horLayout->AddStretchSpacer();
    horLayout->Add(logFile_path, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    horLayout->Add(logFile_button, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);

    wxButton* run_button = new wxButton(mainPanel, ID_RUN_BUTTON, wxString("run"));//, wxDefaultPosition, wxDefaultSize, wxSTRETCH_NOT);

    wxStaticLine* splitLine = new wxStaticLine(mainPanel, wxID_ANY, wxPoint(0, 0), wxSize(460,2));

    isp_status = new wxStaticText(mainPanel, ID_ISP_STATUS, wxString("ISP NOT RUN"));//,  wxDefaultPosition, wxDefaultSize, 
        //wxALIGN_CENTRE_HORIZONTAL|wxBORDER_STATIC, wxString::FromAscii(wxStaticTextNameStr));
    wxFont cur_font = isp_status->GetFont();
    cur_font.SetPointSize(36);
    isp_status->SetFont(cur_font);

    verLayout->Add(horLayout, 1, 0);
    verLayout->Add(splitLine, 0, wxEXPAND, 8);
    verLayout->Add(run_button, 1, wxALIGN_CENTER_HORIZONTAL, 0);
    verLayout->Add(isp_status, 4, wxALIGN_CENTER_HORIZONTAL, 0);
    // verLayout->AddStretchSpacer(4);

    mainPanel->SetSizer(verLayout);
    Centre();

    Connect(ID_RUN_BUTTON, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(mainFrame::OnRunClick));
    Connect(ID_CFG_FILE_BUTTON, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(mainFrame::OnSelectXmlClick));
    Connect(ID_LOG_FILE_BUTTON, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(mainFrame::OnSelectLogClick));

    SetMinSize(wxSize(800, 480));
}

void mainFrame::OnRunClick(wxCommandEvent& event)
{
    wxString cur_pipe_id = pipeID_list->GetStringSelection();
    wxString cur_frame_cnt = frameCnt_list->GetStringSelection();
    wxString xml_path = cfgFile_path->GetLabelText();
    wxString log_file_path = logFile_path->GetLabelText();
    int pipe_id = 0;
    cur_pipe_id.ToInt(&pipe_id);
    int frame_cnt = 0;
    cur_frame_cnt.ToInt(&frame_cnt);
#ifdef _MSC_VER
    static bool cwd_update = false;
    wxString cwd = wxFileName::GetCwd();
    if(!cwd_update)
    {
        cwd.append("/../../build");
        if(!wxFileName::DirExists(cwd))
        {
            wxString msg;
            msg.Printf("%s doesn't exists", cwd);
            wxMessageBox(msg);
            return;
        }
        wxFileName::SetCwd(cwd);
        cwd_update = true;
    }
#endif

    m_pThread = new MyThread(this, pipe_id,log_file_path, xml_path, frame_cnt);
    if(m_pThread->Create() != wxTHREAD_NO_ERROR)
    {
        wxLogError("Can't create the thread!");
        delete m_pThread;
        m_pThread = nullptr;
        isp_status->SetLabel("Error");
        wxSize minsize = verLayout->CalcMin();
        verLayout->RepositionChildren(minsize);
        return;
    }
    if(m_pThread->Run() != wxTHREAD_NO_ERROR)
    {
        wxLogError("Can't create the thread!");
        delete m_pThread;
        m_pThread = nullptr;
        isp_status->SetLabel("Error");
        wxSize minsize = verLayout->CalcMin();
        verLayout->RepositionChildren(minsize);
        return;
    }
    isp_status->SetLabel("Running...");
    wxSize minsize = verLayout->CalcMin();
    verLayout->RepositionChildren(minsize);
}

void mainFrame::OnSelectXmlClick(wxCommandEvent &event)
{
    wxString wildcard = wxT("xml files (*.xml)|*.xml|*.XML");
    wxString defaultDir = wxGetCwd();
    wxString defaultFilename = wxEmptyString;
    wxFileDialog dialog(this, wxT("Choose a file"), defaultDir, defaultFilename, wildcard, wxFD_OPEN|wxFD_FILE_MUST_EXIST);
    if (dialog.ShowModal() == wxID_OK)
    {
        cfgFile_path->SetEditable(false);
#ifdef _MSC_VER
        cfgFile_path->SetLabelText(dialog.GetPath());
#else
        cfgFile_path->ChangeValue(dialog.GetPath());
#endif
    }
}

void mainFrame::OnSelectLogClick(wxCommandEvent& event)
{
    wxString wildcard = wxT("log files (*.txt)|*.txt|*.TXT");
    wxString defaultDir = wxGetCwd();
    wxString defaultFilename = wxEmptyString;
    wxFileDialog dialog(this, wxT("Choose or create a file"), defaultDir, defaultFilename, wildcard, wxFD_OPEN|wxFD_FILE_MUST_EXIST);
    if (dialog.ShowModal() == wxID_OK)
    {
        logFile_path->SetEditable(false);
#ifdef _MSC_VER
        logFile_path->SetLabelText(dialog.GetPath());
#else
        logFile_path->ChangeValue(dialog.GetPath());
#endif
    }
}

void mainFrame::OnThreadCompletion(wxCommandEvent& event)
{
    if(event.GetInt())
    {
        isp_status->SetLabel("Success");
        wxSize minsize = verLayout->CalcMin();
        verLayout->RepositionChildren(minsize);
    }
    else{
        isp_status->SetLabel("Fail");
        wxSize minsize = verLayout->CalcMin();
        verLayout->RepositionChildren(minsize);
    }
}
 
void mainFrame::OnThreadUpdate(wxCommandEvent& event)
{
    wxMessageOutputDebug().Printf("MYFRAME: MyThread update...\n");
}

void mainFrame::OnClose(wxCloseEvent& event)
{
    {
        wxCriticalSectionLocker enter(m_pThreadCS);
 
        if (m_pThread)         // does the thread still exist?
        {
            wxMessageOutputDebug().Printf("MYFRAME: deleting thread");
 
            if (m_pThread->Delete() != wxTHREAD_NO_ERROR )
                wxLogError("Can't delete the thread!");
        }
    }       // exit from the critical section to give the thread
            // the possibility to enter its destructor
            // (which is guarded with m_pThreadCS critical section!)
 
    while (1)
    {
        { // was the ~MyThread() function executed?
            wxCriticalSectionLocker enter(m_pThreadCS);
            if (!m_pThread) break;
        }
 
        // wait for thread completion
        wxThread::This()->Sleep(1);
    }
 
    Destroy();
}



class MyApp : public wxApp
{
  public:
    virtual bool OnInit();
};

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
    mainFrame *simple = new mainFrame(wxT("ISP simulation"));
    simple->Show(true);

    return true;
}