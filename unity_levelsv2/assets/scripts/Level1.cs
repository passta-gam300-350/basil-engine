using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;
using System.Numerics;
using BasilEngine.SceneManagement;

public class Level1 : Behavior
{
    private Audio entranceVO;
    private Audio kiteVO;

    private bool voStarted = false;
    private bool kiteVOStarted = false;

    private GameManager gameManager;
    private bool sceneLoaded = false;

    private float kiteVOTimer = 0f;
    public float kiteVOLength = 4f;

    public void Init()
    {

        // Find entrance VO object
        GameObject voObj = GameObject.Find("EntranceVO");
        if (voObj != null)
        {
            entranceVO = voObj.transform.GetComponent<Audio>();

            if (entranceVO == null)
            {
                Logger.Warn("EntranceVO found but Audio component missing!");
            }
        }
        else
        {
            Logger.Warn("EntranceVO object not found!");
        }

        GameObject kiteObj = GameObject.Find("meng_kiteVO");
        if (kiteObj != null)
        {
            kiteVO = kiteObj.transform.GetComponent<Audio>();
        }
        else
        {
            Logger.Warn("meng_kiteVO not found!");
        }

        gameManager = GameManager.instance;

        if (gameManager == null)
        {
            Logger.Warn("GameManager instance not found!");
        }
    }

    public void Update()
    {
        // play VO once
        if (!voStarted && entranceVO != null)
        {
            entranceVO.Play();
            voStarted = true;
        }

        if (gameManager != null && gameManager.KitePuzzleCompleted && !kiteVOStarted)
        {
            kiteVOStarted = true;

            if (kiteVO != null)
                kiteVO.Play();
        }

        // After kite VO -> load scene
        if (kiteVOStarted && !sceneLoaded)
        {
            kiteVOTimer += Time.deltaTime;

            if (kiteVOTimer >= kiteVOLength)
            {
                sceneLoaded = true;
                Logger.Log("Loading Kite Level...");
                Scene.LoadScene(4);
            }
        }

    }


    public void FixedUpdate()
    {
    }
}
