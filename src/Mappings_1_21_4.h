#pragma once

// Intermediary Mappings for Minecraft 1.21.4 Fabric
namespace Mappings {
    namespace Minecraft {
        inline const char* ClassName = "net/minecraft/class_310"; // MinecraftClient
        inline const char* DeobfName = "net/minecraft/client/MinecraftClient";
        inline const char* GetInstance = "method_1551"; // getInstance
        inline const char* GetInstanceSig = "()Lnet/minecraft/class_310;";
        
        inline const char* PlayerField = "field_1724"; // player
        inline const char* PlayerSig = "Lnet/minecraft/class_746;";
        
        inline const char* WorldField = "field_1687"; // world
        inline const char* WorldSig = "Lnet/minecraft/class_638;";
    }

    namespace Entity {
        inline const char* ClassName = "net/minecraft/class_1297"; // Entity
        inline const char* DeobfName = "net/minecraft/entity/Entity";
        
        inline const char* GetX = "method_23317"; // getX
        inline const char* GetY = "method_23318"; // getY
        inline const char* GetZ = "method_23321"; // getZ
        inline const char* GetCoordsSig = "()D";
        
        inline const char* GetYaw = "method_36454"; // getYaw
        inline const char* GetPitch = "method_36455"; // getPitch
        inline const char* GetRotationSig = "()F";
        
        inline const char* SetYaw = "method_36456"; // setYaw
        inline const char* SetPitch = "method_36457"; // setPitch
        inline const char* SetRotationSig = "(F)V";

        inline const char* EyeHeight = "method_5751"; // getStandingEyeHeight
        inline const char* EyeHeightSig = "()F";
    }

    namespace ClientWorld {
        inline const char* ClassName = "net/minecraft/class_638"; // ClientWorld
        inline const char* GetEntities = "method_18112"; // getEntities
        inline const char* GetPlayers = "method_18456"; // getPlayers
        inline const char* GetPlayersSig = "()Ljava/util/List;"; 
    }

    namespace Java {
        namespace List {
            inline const char* ClassName = "java/util/List";
            inline const char* Iterator = "iterator";
            inline const char* IteratorSig = "()Ljava/util/Iterator;";
        }
        namespace Iterator {
            inline const char* ClassName = "java/util/Iterator";
            inline const char* HasNext = "hasNext";
            inline const char* HasNextSig = "()Z";
            inline const char* Next = "next";
            inline const char* NextSig = "()Ljava/lang/Object;";
        }
    }
}
