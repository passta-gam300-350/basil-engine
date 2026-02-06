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

    private Audio tutorialEndVO;

    private bool endingVOStarted = false;
    private float endVOTimer = 0f;

    public float endVODuration = 2.5f;

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

        GameObject endVOObj = GameObject.Find("TutorialEndVO");

        if (endVOObj != null)
        {
            tutorialEndVO = endVOObj.transform.GetComponent<Audio>();
        }
        else
        {
            Logger.Warn("TutorialEndVO object not found!");
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

        if (endingVOStarted)
        {
            endVOTimer -= Time.deltaTime;

            if (endVOTimer <= 0f)
            {
                Logger.Log("Ending VO finished. Loading Level 1...");

                GameManager.instance.isCleaned = false;
                Scene.LoadScene(3);
            }
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

        if (tutorialEndVO != null)
        {
            tutorialEndVO.Play();
            endingVOStarted = true;
            endVOTimer = endVODuration;
        }
        else
        {
            // fallback if no VO
            Scene.LoadScene(3);
        }
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
