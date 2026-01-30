using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;
using System.Security.AccessControl;

public class GhostBehavior : Behavior
{
    // PUBLIC CONFIGURATION - Set these in the editor
    // Waypoint GameObjects - leave unused ones as null
    public GameObject waypoint1;
    public GameObject waypoint2;
    public GameObject waypoint3;
    public GameObject waypoint4;
    public GameObject waypoint5;
    public GameObject waypoint6;
    public GameObject waypoint7;
    public GameObject waypoint8;
    public GameObject waypoint9;
    public GameObject waypoint10;

    // Pause durations at each waypoint (in seconds)
    public float pauseDuration1 = 1.0f;
    public float pauseDuration2 = 1.0f;
    public float pauseDuration3 = 1.0f;
    public float pauseDuration4 = 1.0f;
    public float pauseDuration5 = 1.0f;
    public float pauseDuration6 = 1.0f;
    public float pauseDuration7 = 1.0f;
    public float pauseDuration8 = 1.0f;
    public float pauseDuration9 = 1.0f;
    public float pauseDuration10 = 1.0f;

    // Movement settings
    public float moveSpeed = 2.0f;           // Movement speed (units per second)
    public float arrivalThreshold = 0.1f;    // Distance to waypoint to consider "arrived"

    // PRIVATE STATE
    private int currentWaypointIndex = 0;    // Which waypoint we're targeting (0-based)
    private int waypointCount = 0;           // Total number of valid waypoints
    private GhostState currentState = GhostState.Moving;
    private float pauseTimer = 0.0f;         // Timer for pause duration
    private Rigidbody rb;                    // Rigidbody component for physics-based movement

    private enum GhostState
    {
        Moving,     // Moving towards waypoint
        Paused      // Waiting at waypoint
    }

    public void Init()
    {
        // Get the Rigidbody component for physics-based movement
        rb = GetComponent<Rigidbody>();
        if (rb == null)
        {
            Logger.Warn("GhostBehavior: No Rigidbody component found! Movement may not work correctly with physics.");
        }

        waypoint1 = GameObject.Find("1");
        waypoint2 = GameObject.Find("2");
        waypoint3 = GameObject.Find("3");
        waypoint4 = GameObject.Find("4");


        // Count how many waypoints are assigned
        waypointCount = CountWaypoints();

        if (waypointCount == 0)
        {
            Logger.Warn("GhostBehavior: No waypoints assigned! Ghost will not move.");
            return;
        }

        // Start at first waypoint
        currentWaypointIndex = 0;
        currentState = GhostState.Moving;
        pauseTimer = 0.0f;
        Logger.Log($"GhostBehavior initialized with {waypointCount} waypoints.");
    }

    public void Update()
    {
        // Don't do anything if no waypoints configured
        if (waypointCount == 0)
        {
            return;
        }

        switch (currentState)
        {
            case GhostState.Moving:
                UpdateMovement();
                break;

            case GhostState.Paused:
                UpdatePause();
                break;
        }
    }

    public void FixedUpdate()
    {
        // Not using fixed update for now
    }

    /// <summary>
    /// Count how many waypoints are assigned
    /// </summary>
    private int CountWaypoints()
    {
        int count = 0;
        if (waypoint1 != null) count++;
        if (waypoint2 != null) count++;
        if (waypoint3 != null) count++;
        if (waypoint4 != null) count++;
        if (waypoint5 != null) count++;
        if (waypoint6 != null) count++;
        if (waypoint7 != null) count++;
        if (waypoint8 != null) count++;
        if (waypoint9 != null) count++;
        if (waypoint10 != null) count++;
        return count;
    }

    /// <summary>
    /// Get waypoint at specified index (0-based)
    /// </summary>
    private GameObject GetWaypoint(int index)
    {
        switch (index)
        {
            case 0: return waypoint1;
            case 1: return waypoint2;
            case 2: return waypoint3;
            case 3: return waypoint4;
            case 4: return waypoint5;
            case 5: return waypoint6;
            case 6: return waypoint7;
            case 7: return waypoint8;
            case 8: return waypoint9;
            case 9: return waypoint10;
            default: return null;
        }
    }

    /// <summary>
    /// Get pause duration at specified index (0-based)
    /// </summary>
    private float GetPauseDuration(int index)
    {
        switch (index)
        {
            case 0: return pauseDuration1;
            case 1: return pauseDuration2;
            case 2: return pauseDuration3;
            case 3: return pauseDuration4;
            case 4: return pauseDuration5;
            case 5: return pauseDuration6;
            case 6: return pauseDuration7;
            case 7: return pauseDuration8;
            case 8: return pauseDuration9;
            case 9: return pauseDuration10;
            default: return 1.0f;
        }
    }

    /// <summary>
    /// Update movement towards current waypoint
    /// </summary>
    private void UpdateMovement()
    {
        GameObject targetWaypoint = GetWaypoint(currentWaypointIndex);
        if (targetWaypoint == null)
        {
            Logger.Warn($"GhostBehavior: Waypoint at index {currentWaypointIndex} is null! Skipping...");
            AdvanceToNextWaypoint();
            return;
        }

        Vector3 targetPosition = targetWaypoint.transform.position;
        Vector3 currentPosition = transform.position;

        // Calculate direction and distance
        Vector3 direction = targetPosition - currentPosition;
        float distance = direction.Magnitude();

        // Check if we've arrived at the waypoint
        if (distance <= arrivalThreshold)
        {
            OnArriveAtWaypoint();
            return;
        }

        // Move towards the target waypoint using physics
        float stepDistance = moveSpeed * Time.deltaTime;
        Vector3 newPosition;

        if (stepDistance >= distance)
        {
            // We'll reach or overshoot the target this frame, so just snap to it
            newPosition = targetPosition;
        }
        else
        {
            // Move a fraction of the way
            Vector3 normalizedDirection = direction.Normalize();
            newPosition = currentPosition + (normalizedDirection * stepDistance);
        }

        // Use Rigidbody.MovePosition for physics-based movement
        // This ensures proper collision detection and physics integration
        if (rb != null)
        {
            rb.MovePosition(newPosition);
        }
        else
        {
            // Fallback to direct transform manipulation if no rigidbody
            transform.position = newPosition;
        }
    }

    /// <summary>
    /// Update pause timer at waypoint
    /// </summary>
    private void UpdatePause()
    {
        // Ensure velocity stays at zero while paused
        if (rb != null)
        {
            rb.velocity = new Vector3(0, 0, 0);
        }

        pauseTimer -= Time.deltaTime;

        if (pauseTimer <= 0.0f)
        {
            // Pause finished, move to next waypoint
            AdvanceToNextWaypoint();
            currentState = GhostState.Moving;
        }
    }

    /// <summary>
    /// Called when ghost arrives at a waypoint
    /// </summary>
    private void OnArriveAtWaypoint()
    {
        // Start pause at this waypoint
        currentState = GhostState.Paused;
        pauseTimer = GetPauseDuration(currentWaypointIndex);

        // Stop the rigidbody's velocity to prevent continued movement
        if (rb != null)
        {
            rb.velocity = new Vector3(0, 0, 0);
        }

        Logger.Log($"GhostBehavior: Arrived at waypoint {currentWaypointIndex + 1}, pausing for {pauseTimer} seconds.");
    }

    /// <summary>
    /// Move to the next waypoint in the sequence
    /// </summary>
    private void AdvanceToNextWaypoint()
    {
        currentWaypointIndex++;

        // Loop back to start if we've reached the end
        if (currentWaypointIndex >= waypointCount)
        {
            currentWaypointIndex = 0;
        }

        Logger.Log($"GhostBehavior: Moving to waypoint {currentWaypointIndex + 1}.");
    }
}
