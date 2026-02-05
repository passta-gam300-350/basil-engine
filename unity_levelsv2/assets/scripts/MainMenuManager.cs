using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using System;



public class MainMenuManager : Behavior
{
    enum MenuStates
    {

        PLAY_SELECT,
        QUIT_SELECT
    }
    MenuStates currentState = MenuStates.PLAY_SELECT;


    private GameObject[] menus = new GameObject[2];
    private HudComponent play;

    private bool upKeyDebounce, downKeyDebounce;
    public void Init()
    {
        menus[0] = GameObject.Find("GUIBackground_Play");
        menus[1] = GameObject.Find("GUIBackground_Quit");

        play = menus[0].transform.GetComponent<HudComponent>();


    }
    public void Update()
    {
        if ((Input.GetKey(KeyCode.DOWN) || Input.GetKey(KeyCode.S)) && !downKeyDebounce)
        {
            downKeyDebounce = true;
            if (currentState == MenuStates.PLAY_SELECT)
            {
                currentState = MenuStates.QUIT_SELECT;
            }
            else currentState = MenuStates.PLAY_SELECT;
        }
        else if (!Input.GetKey(KeyCode.DOWN) && !Input.GetKey(KeyCode.S))
        {
            downKeyDebounce = false;
        }




        if ((Input.GetKeyPress(KeyCode.UP) || Input.GetKeyPress(KeyCode.W)) && !upKeyDebounce)
        {


            upKeyDebounce = true;


            if (currentState == MenuStates.QUIT_SELECT)
            {
                currentState = MenuStates.PLAY_SELECT;
            }
            else currentState = MenuStates.QUIT_SELECT;
        }
        else if (!Input.GetKey(KeyCode.UP) && !Input.GetKey(KeyCode.W))
        {
            upKeyDebounce = false;
        }

        switch (currentState)
        {
            case MenuStates.PLAY_SELECT:
                play.Visible = true;
                break;
            case MenuStates.QUIT_SELECT:
                play.Visible = false;
                break;

        }

    }

    public void FixedUpdate()
    {

    }


}
