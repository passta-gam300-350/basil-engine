using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using System;
using BasilEngine.SceneManagement;


public class MainMenuManager : Behavior
{
    enum MenuStates
    {

        PLAY_SELECT,
        SETTINGS_SELECT,
        QUIT_SELECT
    }
    MenuStates currentState = MenuStates.PLAY_SELECT;


    private GameObject[] menus = new GameObject[3];
    private HudComponent play,settings,quit;


    private Audio audio;
    private float fadeSpeed = 100.0f;
    private bool isFadingIn = false;
    private float targetVolume;
    private bool settings_menu = false;

    public void Init()
    {
        menus[0] = GameObject.Find("GUIBackground_Play");
        menus[1] = GameObject.Find("GUIBackground_Settings");
        menus[2] = GameObject.Find("GUIBackground_Quit");

        play = menus[0].transform.GetComponent<HudComponent>();
        settings = menus[1].transform.GetComponent<HudComponent>();
        quit = menus[2].transform.GetComponent<HudComponent>();

        audio = transform.GetComponent<Audio>();
        targetVolume = audio.Volume;
        audio.Volume = 0;
        audio.Play();
        isFadingIn = true;

        Input.CursorHidden = true;
    }
    public void Update()
    {

        if (isFadingIn)
        {
            audio.Volume += (targetVolume * fadeSpeed / 100.0f) * Time.deltaTime;

            if (audio.Volume >= targetVolume)
            {
                audio.Volume = targetVolume;
                isFadingIn = false;
            }
        }
        //if ((Input.GetKey(KeyCode.DOWN) || Input.GetKey(KeyCode.S)) && !downKeyDebounce)
        //{
        //    downKeyDebounce = true;
        //    if (currentState == MenuStates.PLAY_SELECT)
        //    {
        //        currentState = MenuStates.QUIT_SELECT;
        //    }
        //    else currentState = MenuStates.PLAY_SELECT;
        //}
        //else if (!Input.GetKey(KeyCode.DOWN) && !Input.GetKey(KeyCode.S))
        //{
        //    downKeyDebounce = false;
        //}

        if (Input.GetKeyPress(KeyCode.DOWN) || Input.GetKeyPress(KeyCode.S))
        {
            
            // Execute action for down key press
            if (currentState == MenuStates.PLAY_SELECT)
            {
                currentState = MenuStates.SETTINGS_SELECT;
            }
            else if(currentState == MenuStates.SETTINGS_SELECT)
            {
                currentState = MenuStates.QUIT_SELECT;
            }
            else currentState = MenuStates.PLAY_SELECT;
        }

        if (Input.GetKeyPress(KeyCode.UP) || Input.GetKeyPress(KeyCode.W))
        {
            // Execute action for up key press
            if (currentState == MenuStates.QUIT_SELECT)
            {
                currentState = MenuStates.SETTINGS_SELECT;
            }
            else if (currentState == MenuStates.SETTINGS_SELECT)
            {
                currentState = MenuStates.PLAY_SELECT;
            }
            else currentState = MenuStates.QUIT_SELECT;
        }


        //if ((Input.GetKeyPress(KeyCode.UP) || Input.GetKeyPress(KeyCode.W)) && !upKeyDebounce)
        //{


        //    upKeyDebounce = true;


        //    if (currentState == MenuStates.QUIT_SELECT)
        //    {
        //        currentState = MenuStates.PLAY_SELECT;
        //    }
        //    else currentState = MenuStates.QUIT_SELECT;
        //}
        //else if (!Input.GetKey(KeyCode.UP) && !Input.GetKey(KeyCode.W))
        //{
        //    upKeyDebounce = false;
        //}

        switch (currentState)
        {
            case MenuStates.PLAY_SELECT:
                play.Visible = true;
                settings.Visible = false;
                quit.Visible = false;
                break;
            case MenuStates.SETTINGS_SELECT:
                play.Visible = false;
                settings.Visible = true;
                quit.Visible = false;
                break;
            case MenuStates.QUIT_SELECT:
                play.Visible = false;
                settings.Visible = false;
                quit.Visible = true;
                break;

        }


        if (Input.GetKeyPress(KeyCode.ENTER) || Input.GetKeyPress(KeyCode.SPACE))
        {
            switch (currentState)
            {
                case MenuStates.PLAY_SELECT:
                    Scene.LoadSceneByIndex(1);
                    break;
                case MenuStates.SETTINGS_SELECT:
                    settings_menu = true;
                    break;
                case MenuStates.QUIT_SELECT:
                    Scene.Quit();
                    break;
            }
        }

        if (settings_menu)
        {
            //settings_page.Visible = true;
        }
    }

    public void FixedUpdate()
    {

    }


}
