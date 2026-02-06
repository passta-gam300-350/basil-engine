using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using System;
using System.Collections.Generic;
using BasilEngine.SceneManagement;


public class PauseMenuManager : Behavior
{

    enum PauseMenuType
    {
        MAIN,
        RESTART,
        QUIT
    }
    enum PauseMenuState
    {
        UNPAUSED,
        MENU_RESUME,
        MENU_RESTART,
        MENU_QUIT,
        OPTION_YES,
        OPTION_NO
    }

    Dictionary<PauseMenuType, List<PauseMenuState>> menuStates = new Dictionary<PauseMenuType, List<PauseMenuState>>()
    {
        { PauseMenuType.MAIN, new List<PauseMenuState> { PauseMenuState.MENU_RESUME, PauseMenuState.MENU_RESTART,PauseMenuState.MENU_QUIT } },
        { PauseMenuType.RESTART, new List<PauseMenuState> { PauseMenuState.OPTION_NO, PauseMenuState.OPTION_YES } },
        { PauseMenuType.QUIT, new List<PauseMenuState> { PauseMenuState.OPTION_NO, PauseMenuState.OPTION_YES } }
    };

    private List<GameObject> UIObjects;
    private List<HudComponent> HudComponents;
    Dictionary<PauseMenuType, Dictionary<PauseMenuState, GameObject>> stateObjects = new Dictionary<PauseMenuType, Dictionary<PauseMenuState, GameObject>>();

    PauseMenuType currentMenuType = PauseMenuType.MAIN;
    PauseMenuState currentMenuState = PauseMenuState.UNPAUSED;

    private KeyCode previous = KeyCode.A;
    private KeyCode next = KeyCode.D;
    private KeyCode previous2 = KeyCode.LEFT;
    private KeyCode next2 = KeyCode.RIGHT;

    private KeyCode select = KeyCode.ENTER;
    private KeyCode select2 = KeyCode.SPACE;

    private KeyCode exit = KeyCode.ESCAPE;

    private int start = 0, end = 0;
    private bool onTypeChanged = false;
    public void Init()
    {
        UIObjects =
        [
            GameObject.Find("PauseMenu_Menu_Resume"),
            GameObject.Find("PauseMenu_Menu_Restart"),
            GameObject.Find("PauseMenu_Menu_MainMenu"),
            GameObject.Find("PauseMenu_Restart_Yes"),
            GameObject.Find("PauseMenu_Restart_No"),
            GameObject.Find("PauseMenu_Quit_Yes"),
            GameObject.Find("PauseMenu_Quit_No")
        ];
        HudComponents = new List<HudComponent>();

        foreach (GameObject obj in UIObjects)
        {
            HudComponents.Add(obj.transform.GetComponent<HudComponent>());
        }


        stateObjects.Add(PauseMenuType.MAIN, new Dictionary<PauseMenuState, GameObject>()
        {
            { PauseMenuState.MENU_RESUME, GameObject.Find("PauseMenu_Menu_Resume") },
            { PauseMenuState.MENU_RESTART, GameObject.Find("PauseMenu_Menu_Restart") },
            { PauseMenuState.MENU_QUIT, GameObject.Find("PauseMenu_Menu_MainMenu") }
        });
        stateObjects.Add(PauseMenuType.RESTART, new Dictionary<PauseMenuState, GameObject>()
        {
            { PauseMenuState.OPTION_YES, GameObject.Find("PauseMenu_Restart_Yes") },
            { PauseMenuState.OPTION_NO, GameObject.Find("PauseMenu_Restart_No") }
        });
        stateObjects.Add(PauseMenuType.QUIT, new Dictionary<PauseMenuState, GameObject>()
        {
            { PauseMenuState.OPTION_YES, GameObject.Find("PauseMenu_Quit_Yes") },
            { PauseMenuState.OPTION_NO, GameObject.Find("PauseMenu_Quit_No") }
        });
    }



    public void Update()
    {
        if (Input.GetKeyPress(KeyCode.ESCAPE))
        {
            currentMenuState = currentMenuState != PauseMenuState.UNPAUSED ? PauseMenuState.UNPAUSED : PauseMenuState.MENU_RESUME;
            Logger.Log(currentMenuState.ToString());
            currentMenuType = PauseMenuType.MAIN;
            GameManager.IsGamePaused = true;
        }

        if (currentMenuState == PauseMenuState.UNPAUSED)
        {
            foreach (HudComponent component in HudComponents)
            {
                if (component.Visible == true)
                {
                    component.Visible = false;
                }
                else continue;

            }
        }

        if (currentMenuState == PauseMenuState.UNPAUSED)
            return;

        if (currentMenuType == PauseMenuType.MAIN)
            {
                end = menuStates[PauseMenuType.MAIN].Count;
                start = (onTypeChanged) ? 0 : start;
                onTypeChanged = false;

            }


            if (Input.GetKeyPress(next) || Input.GetKeyPress(next2))
            {
                start++;


            }
            if (Input.GetKeyPress(previous) || Input.GetKeyPress(previous2))
            {
                start--;

            }

            if (start >= end)
            {
                start = 0;
            }

            if (start < 0)
            {
                start = end - 1;
            }

            currentMenuState = menuStates[currentMenuType][start];




            int counter = 0;
            GameObject current = GetCurrentSelectedObject();
            ulong name = current.NativeID;


            foreach (GameObject obj in UIObjects)
            {
                if (obj.NativeID != name)
                {
                    if (HudComponents[counter] != null)
                    {
                        HudComponents[counter].Visible = false;
                    }
                }
                else
                {
                    HudComponents[counter].Visible = true;
                }
                counter++;
            }

            if (Input.GetKeyPress(select) || Input.GetKeyPress(select2))
            {
                if (currentMenuState == PauseMenuState.MENU_RESUME)
                {
                    currentMenuState = PauseMenuState.UNPAUSED;
                }

                if (currentMenuState == PauseMenuState.MENU_RESTART)
                {
                    currentMenuType = PauseMenuType.RESTART;
                    onTypeChanged = true;
                    start = 0;
                    end = menuStates[currentMenuType].Count;
                }
                else if (currentMenuState == PauseMenuState.MENU_QUIT)
                {
                    currentMenuType = PauseMenuType.QUIT;
                    onTypeChanged = true;
                    start = 0;
                    end = menuStates[currentMenuType].Count;
                }

                if (currentMenuType == PauseMenuType.RESTART && currentMenuState == PauseMenuState.OPTION_YES)
                {
                    Logger.Log("Reload Scene");
                }
                else if (currentMenuType == PauseMenuType.RESTART && currentMenuState == PauseMenuState.OPTION_NO)
                {
                    currentMenuType = PauseMenuType.MAIN;
                    onTypeChanged = true;
                    start = 0;
                    end = menuStates[currentMenuType].Count;
                }

                if (currentMenuType == PauseMenuType.QUIT && currentMenuState == PauseMenuState.OPTION_YES)
                {
                    Scene.LoadScene(0);
                }
                else if (currentMenuType == PauseMenuType.QUIT && currentMenuState == PauseMenuState.OPTION_NO)
                {
                    currentMenuType = PauseMenuType.MAIN;
                    onTypeChanged = true;
                    start = 0;
                    end = menuStates[currentMenuType].Count;
                }


            }



        }

    public void FixedUpdate()
    {

    }


    public GameObject GetCurrentSelectedObject()
    {

        return stateObjects[currentMenuType][currentMenuState];
    }


}
