using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;
using System.Numerics;
using BasilEngine.SceneManagement;

public class TutorialLevel : Behavior
{
    private Audio tutorialIntroVO;
    //private GameObject pickupUI;

    private bool voStarted = false;

    private bool tutorialFinished = false;

    //private bool uiShown = false;
    //private float voTimer = 0f;

    //public float voDuration = 6.0f;


    public void Init()
    {

        // Find tutorial VO object
        GameObject voObj = GameObject.Find("TutorialIntroVO");
        if (voObj != null)
        {
            tutorialIntroVO = voObj.transform.GetComponent<Audio>();

            if (tutorialIntroVO == null)
            {
                Logger.Warn("TutorialIntroVO found but Audio component missing!");
            }
        }
        else
        {
            Logger.Warn("TutorialIntroVO object not found!");
        }

        //// Find Pickup UI
        //pickupUI = GameObject.Find("PickupUI");
        //if (pickupUI != null)
        //{
        //    pickupUI.visibility = false; // hide at start
        //}
        //else
        //{
        //    Logger.Warn("PickupUI object not found!");
        //}
    }

    public void Update()
    {
        // Play VO once
        if (!voStarted && tutorialIntroVO != null)
        {
            tutorialIntroVO.Play();
            voStarted = true;
            //voTimer = voDuration;

            Logger.Log("Tutorial intro VO started");
        }

        // play VO once
        if (!voStarted && tutorialIntroVO != null)
        {
            tutorialIntroVO.Play();
            voStarted = true;
            Logger.Log("Tutorial intro VO started");
        }

        // tutorial completion check
        if (GameManager.instance != null && GameManager.instance.isCleaned)
        {
            CompleteTutorial();
        }

        //// Wait for VO to finish
        //if (voStarted && !uiShown)
        //{
        //    voTimer -= Time.deltaTime;

        //    if (voTimer <= 0f)
        //    {
        //        ShowPickupUI();
        //    }
        //}

    }

    private void CompleteTutorial()
    {
        if (tutorialFinished) return;

        tutorialFinished = true;

        Logger.Log("Tutorial complete! Loading Level 1...");

        // reset tutorial-only flags if needed
        GameManager.instance.isCleaned = false;

        Scene.LoadScene(3);
    }

    //private void ShowPickupUI()
    //{
    //    if (pickupUI != null)
    //    {
    //        pickupUI.visibility = true;
    //        Logger.Log("Pickup UI shown");
    //    }

    //    uiShown = true;
    //}

    public void FixedUpdate()
    {
    }
}
