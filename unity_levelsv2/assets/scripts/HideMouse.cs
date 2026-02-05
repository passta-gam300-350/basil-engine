using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using System;



public class HideMouse : Behavior
{
    
    KeyCode hideKey = KeyCode.H;
    private bool hidden = false;
    public void Init()
    {
        hidden = true;
        Input.CursorHidden = hidden;
    }
    public void Update()
    {
        if (Input.GetKeyPress(hideKey))
        {
            hidden = !hidden;
            Input.CursorHidden = hidden;
        }
    }

    public void FixedUpdate()
    {

    }


}