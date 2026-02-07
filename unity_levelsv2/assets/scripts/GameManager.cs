using System.Linq.Expressions;
using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using BasilEngine.SceneManagement;
using System;


public class GameManager : Behavior
{


    public static GameManager instance;
    public bool TrainPuzzleCompleted = false;
    public bool KitePuzzleCompleted = false;
    public bool isCleaned = false;



    private GameObject player;
    private Rigidbody playerRigidbody;
    private PlayerController3D controller;

    //public GameObject droppedBagPrefab;
    public static bool IsGamePaused = false;

    public void Init()
    {
        instance = this;

        player = GameObject.Find("PlayerGroup");

        if (player != null)
        {
            playerRigidbody = player.transform.GetComponent<Rigidbody>();

            if (playerRigidbody == null)
            {
                Logger.Warn("Player Rigidbody not found!");
            }

            controller = player.transform.GetComponent<PlayerController3D>();

            if (controller == null)
            {
                Logger.Warn("PlayerController3D not found!");
            }
        }
        else
        {
            Logger.Warn("PlayerGroup not found; skipping player setup.");
        }

        //droppedBagPrefab = GameObject.Find("DroppedTrashBag");

        //if (droppedBagPrefab == null)
        //{
        //    Logger.Warn("DroppedTrashBag prefab not found!");
        //}
        //else
        //{
        //    droppedBagPrefab.visibility = false; // keep template hidden
        //}
    }
    public void Update()
    {

    }

    public void FixedUpdate()
    {

    }

    public void DisableControls()
    {
        if (controller != null)
        {
            controller.disabled = true;
        }

        if (playerRigidbody != null)
        {
            playerRigidbody.FreezeX = true;
            playerRigidbody.FreezeZ = true;
        }

    }
    public void EnableControls()
    {
        if (controller != null)
        {
            controller.disabled = false;
        }

        if (playerRigidbody != null)
        {
            playerRigidbody.FreezeX = false;
            playerRigidbody.FreezeZ = false;
        }
    }

    public void CompleteTrainPuzzle()
    {
        TrainPuzzleCompleted = true;
    }

    public void CompleteKitePuzzle()
    {
        KitePuzzleCompleted = true;
    }

    public bool GetPaused()
    {
        return IsGamePaused;
    }

    public void PauseGame()
    {
        IsGamePaused = true;
    }
    public void UnpauseGame()
    {
        IsGamePaused = false;
    }

    public void ShowMouse()
    {
        Input.CursorHidden = false;
    }

    public void HideMouse()
    {
        Input.CursorHidden = true;
    }
}